#include "InputReader.h"

#include <Arduino.h>
#include <driver/adc.h>
#include <hal/adc_ll.h>
#include <soc/gpio_reg.h>

#include "Config.h"
#include "Log.h"

namespace InputReader {
namespace {

InputState state;

// Buttons
uint32_t debouncedMask = 0;
uint32_t previousRawMask = 0;
uint32_t lastChangeUs[BUTTON_COUNT] = {};
uint32_t glitchProneMask = 0;

// Sticks
adc1_channel_t axisChannel[AXIS_COUNT];
int axisCenter[AXIS_COUNT];
float deadzone[STICK_COUNT];

uint8_t index(Axis axis) { return static_cast<uint8_t>(axis); }

uint32_t readRawPressedMask() {
  const uint32_t bank0 = REG_READ(GPIO_IN_REG);   // GPIO0..31
  const uint32_t bank1 = REG_READ(GPIO_IN1_REG);  // GPIO32..39 in bits 0..7
  uint32_t mask = 0;
  for (uint8_t i = 0; i < BUTTON_COUNT; ++i) {
    const uint8_t gpio = Config::BUTTON_PINS[i].gpio;
    const bool high = gpio < 32 ? (bank0 >> gpio) & 1u : (bank1 >> (gpio - 32)) & 1u;
    if (!high) mask |= 1u << i;  // active LOW
  }
  return mask;
}

void debounceButtons(uint32_t rawMask, uint32_t nowUs) {
  for (uint8_t i = 0; i < BUTTON_COUNT; ++i) {
    const uint32_t bit = 1u << i;
    const bool rawPressed = rawMask & bit;
    if (rawPressed == static_cast<bool>(debouncedMask & bit)) continue;
    if (nowUs - lastChangeUs[i] < Config::DEBOUNCE_LOCKOUT_US) continue;
    if (rawPressed && (glitchProneMask & bit) && !(previousRawMask & bit)) continue;
    debouncedMask ^= bit;
    lastChangeUs[i] = nowUs;
  }
  previousRawMask = rawMask;
}

// One ADC1 conversion through the RTC controller using the IDF low-level HAL. adc1_get_raw() spends ~45 us per
// call on locking and controller setup (measured: 16 reads = 780 us); the conversion itself takes a few us.
// Safe because nothing else uses ADC1, and begin() leaves it configured (width, attenuation, RTC controller,
// power held on).
int convertAdc1(adc1_channel_t channel) {
  adc_ll_rtc_enable_channel(ADC_NUM_1, channel);
  adc_ll_rtc_start_convert(ADC_NUM_1, channel);
  while (!adc_ll_rtc_convert_is_done(ADC_NUM_1)) {
  }
  return adc_ll_rtc_get_convert_value(ADC_NUM_1);
}

float readAxisRaw(uint8_t a) {
  int32_t sum = 0;
  for (uint8_t i = 0; i < Config::STICK_OVERSAMPLING; ++i) sum += convertAdc1(axisChannel[a]);
  return static_cast<float>(sum) / Config::STICK_OVERSAMPLING;  // keep the sub-count resolution
}

// -1..1 around the power-on center. Each direction is scaled by its own span, so both ends reach full
// deflection even when the center isn't exactly mid-scale.
float normalizeAxis(uint8_t a, float raw) {
  const float delta = raw - axisCenter[a];
  const float span = delta >= 0.0f ? Config::ADC_MAX - axisCenter[a] : axisCenter[a];
  const float n = constrain(delta / span, -1.0f, 1.0f);
  return Config::AXIS_PINS[a].invert ? -n : n;
}

void processStick(Stick stick, Axis axisX, Axis axisY) {
  float x = normalizeAxis(index(axisX), readAxisRaw(index(axisX)));
  float y = normalizeAxis(index(axisY), readAxisRaw(index(axisY)));

  // Each axis saturates on its own, so diagonals would reach the corners of a square; clamp to the circle.
  float magnitude = sqrtf(x * x + y * y);
  if (magnitude > 1.0f) {
    x /= magnitude;
    y /= magnitude;
    magnitude = 1.0f;
  }

  int16_t& outX = state.axes[index(axisX)];
  int16_t& outY = state.axes[index(axisY)];
  const float dz = deadzone[static_cast<uint8_t>(stick)];
  if (magnitude <= dz) {
    outX = outY = 0;
    return;
  }

  // Rescale so the output grows from 0 at the deadzone edge to full at the rim, keeping the direction.
  const float scale = (magnitude - dz) / ((1.0f - dz) * magnitude) * AXIS_MAX;
  outX = static_cast<int16_t>(lroundf(x * scale));
  outY = static_cast<int16_t>(lroundf(y * scale));
}

// Uses the driver's adc1_get_raw() on purpose: besides reading, it routes ADC1 to the RTC controller,
// which convertAdc1() relies on afterwards.
void calibrateCenters() {
  for (uint8_t a = 0; a < AXIS_COUNT; ++a) {
    int32_t sum = 0;
    for (uint16_t i = 0; i < Config::STICK_CENTER_SAMPLES; ++i) sum += adc1_get_raw(axisChannel[a]);
    axisCenter[a] = sum / Config::STICK_CENTER_SAMPLES;
    if (abs(axisCenter[a] - Config::ADC_MID) > Config::STICK_CENTER_MAX_OFFSET) {
      LOG("InputReader: axis %u center %d is implausible (stick touched at power-on?), using %d", a,
          axisCenter[a], Config::ADC_MID);
      axisCenter[a] = Config::ADC_MID;
    }
  }
}

void logCenters() {
  LOG("InputReader: centers LX %d LY %d RX %d RY %d", axisCenter[index(Axis::LeftX)], axisCenter[index(Axis::LeftY)],
      axisCenter[index(Axis::RightX)], axisCenter[index(Axis::RightY)]);
}

}  // namespace

void begin() {
  for (uint8_t i = 0; i < BUTTON_COUNT; ++i) {
    const Config::ButtonPin& pin = Config::BUTTON_PINS[i];
    pinMode(pin.gpio, pin.externalPullup ? INPUT : INPUT_PULLUP);
    if (pin.glitchProne) glitchProneMask |= 1u << i;
  }

  adc1_config_width(ADC_WIDTH_BIT_12);
  for (uint8_t a = 0; a < AXIS_COUNT; ++a) {
    axisChannel[a] = static_cast<adc1_channel_t>(digitalPinToAnalogChannel(Config::AXIS_PINS[a].gpio));
    adc1_config_channel_atten(axisChannel[a], ADC_ATTEN_DB_12);
  }
  // Keep the SAR ADC powered: conversions skip the power-up, and GPIO36/39 don't see the power-up glitch.
  adc_power_acquire();

  calibrateCenters();
  for (uint8_t s = 0; s < STICK_COUNT; ++s) setDeadzone(static_cast<Stick>(s), Config::STICK_DEADZONE_PERCENT);
  logCenters();
}

void recalibrate() {
  calibrateCenters();
  logCenters();
}

const InputState& read(uint32_t nowUs) {
  processStick(Stick::Left, Axis::LeftX, Axis::LeftY);
  processStick(Stick::Right, Axis::RightX, Axis::RightY);

  // Buttons last, so the ~150 us of ADC work doesn't delay presses on their way to the report. Safe for the
  // GPIO36/39 errata: every conversion has finished, and the ADC stays powered, so there is no power-up glitch.
  debounceButtons(readRawPressedMask(), nowUs);
  state.buttons = debouncedMask;
  return state;
}

void setDeadzone(Stick stick, uint8_t percent) {
  deadzone[static_cast<uint8_t>(stick)] = min(percent, Config::STICK_DEADZONE_MAX_PERCENT) / 100.0f;
}

}  // namespace InputReader
