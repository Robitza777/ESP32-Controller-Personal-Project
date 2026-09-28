// Orchestrator: owns the task layout and passes plain data between modules.
//
//   core 1  inputTask  every 1 ms, high priority   InputReader -> ProfileButton / Profiles -> BleOutput
//   core 0  uiTask     every 20 ms, low priority   Display, BatteryMonitor, ConfigMenu, Storage writes, PowerManager
//
// The NimBLE host also runs on core 0, above uiTask, so slow UI work (a full OLED frame is ~23 ms of I2C)
// never delays BLE or input. uiTask is the only user of the I2C bus, so the bus needs no locking.

#include <Arduino.h>
#include <Wire.h>
#include <esp_sleep.h>
#include <esp_system.h>

#include <atomic>

#include "BatteryMonitor.h"
#include "BleOutput.h"
#include "Config.h"
#include "ConfigMenu.h"
#include "Display.h"
#include "InputReader.h"
#include "Log.h"
#include "PowerManager.h"
#include "ProfileButton.h"
#include "Profiles.h"
#include "Storage.h"

namespace {

// What the input task tells the UI task. Written every input cycle, read every UI cycle, under a spinlock
// (a copy of a few bytes, so neither side waits noticeably).
struct Status {
  char profileName[PROFILE_NAME_MAX + 1] = {};
  uint8_t profileNumber = 0;
  uint8_t profileCount = 0;
  uint32_t profileSwitches = 0;
  bool layerActive = false;
  bool toggleActive = false;
  uint32_t lastInputMs = 0;
};

portMUX_TYPE statusLock = portMUX_INITIALIZER_UNLOCKED;
Status sharedStatus;

// Input task only.
uint32_t profileSwitches = 0;
uint32_t lastInputMs = 0;

// ---------------------------------------------------------------------------------------------
// Config menu handoff between the tasks.
//
//   Closed     input task owns the profiles; a Profile hold copies them into menuList and sets Open
//   Open       UI task runs the menu on menuList; input task sends the host a neutral report and forwards inputs
//   Saved      UI task has written menuList to flash; input task takes it into Profiles and sets Closed
//   Discarded  input task keeps its profiles and sets Closed
//
// menuList is only touched by the task the state names, so it needs no lock of its own; the state and the
// forwarded inputs live under statusLock.
enum class MenuState : uint8_t { Closed, Open, Saved, Discarded };

ConfigMenu::Workspace menuList;
MenuState menuState = MenuState::Closed;
ConfigMenu::Input menuInput;       // `pressed` collects presses until the UI task takes them
uint32_t menuPreviousButtons = 0;  // input task only

// UI task only (read once in setup, before the tasks start).
DisplaySettings displaySettings;
PowerSettings powerSettings;

bool recalibrateRequested = false;  // UI task -> input task, under statusLock
std::atomic<bool> poweringOff{false};  // UI task -> input task: stop touching BLE

MenuState readMenuState() {
  portENTER_CRITICAL(&statusLock);
  const MenuState state = menuState;
  portEXIT_CRITICAL(&statusLock);
  return state;
}

void setMenuState(MenuState state) {
  portENTER_CRITICAL(&statusLock);
  menuState = state;
  portEXIT_CRITICAL(&statusLock);
}

void publishStatus() {
  const Profile& profile = Profiles::active();
  Status status;
  memcpy(status.profileName, profile.name, sizeof(status.profileName));
  status.profileNumber = Profiles::activeIndex() + 1;
  status.profileCount = Profiles::count();
  status.profileSwitches = profileSwitches;
  status.layerActive = Profiles::layerActive();
  status.toggleActive = Profiles::toggleActive();
  status.lastInputMs = lastInputMs;

  portENTER_CRITICAL(&statusLock);
  sharedStatus = status;
  portEXIT_CRITICAL(&statusLock);
}

Status readStatus() {
  portENTER_CRITICAL(&statusLock);
  const Status status = sharedStatus;
  portEXIT_CRITICAL(&statusLock);
  return status;
}

// Radial, like the deadzone.
bool stickActive(int16_t x, int16_t y) {
  constexpr int32_t threshold = static_cast<int32_t>(AXIS_MAX) * Config::ACTIVITY_STICK_PERCENT / 100;
  return static_cast<int32_t>(x) * x + static_cast<int32_t>(y) * y >= threshold * threshold;
}

// What keeps the screen and the controller on: any button, or a stick pushed well past its jitter.
bool isActivity(const InputState& input) {
  return input.buttons != 0 || stickActive(input.axis(Axis::LeftX), input.axis(Axis::LeftY)) ||
         stickActive(input.axis(Axis::RightX), input.axis(Axis::RightY));
}

// Settings that live in the profile but are applied by other modules.
void applyActiveProfileSettings() {
  const Profile& profile = Profiles::active();
  for (uint8_t s = 0; s < STICK_COUNT; ++s) InputReader::setDeadzone(static_cast<Stick>(s), profile.deadzonePercent[s]);
  LOG("profile %u/%u: %s", Profiles::activeIndex() + 1, Profiles::count(), profile.name);
}

void handleProfileGesture(const ProfileButton::Event& event) {
  switch (event.gesture) {
    case ProfileButton::Gesture::None: return;
    case ProfileButton::Gesture::Next: Profiles::selectNext(); break;
    case ProfileButton::Gesture::Previous: Profiles::selectPrevious(); break;
    case ProfileButton::Gesture::JumpTo:
      if (event.profileIndex >= Profiles::count()) return;
      Profiles::select(event.profileIndex);
      break;
    case ProfileButton::Gesture::OpenMenu: return;  // handled by openMenu()
  }
  ++profileSwitches;
  applyActiveProfileSettings();
}

// Input task.
void openMenu(const InputState& input) {
  menuList.count = Profiles::count();
  for (uint8_t i = 0; i < menuList.count; ++i) menuList.profiles[i] = Profiles::get(i);
  menuList.active = Profiles::activeIndex();
  menuPreviousButtons = input.buttons;
  portENTER_CRITICAL(&statusLock);
  menuInput = ConfigMenu::Input{};
  menuState = MenuState::Open;
  portEXIT_CRITICAL(&statusLock);
}

// Input task.
void closeMenu() {
  ProfileButton::reset();
  ++profileSwitches;  // the name popup: a clear sign the controller is back in play
  applyActiveProfileSettings();
  setMenuState(MenuState::Closed);
}

// Input task: one cycle while the menu is open (or has just closed).
void menuCycle(MenuState state, const InputState& input, uint32_t nowUs) {
  BleOutput::send(OutputReport{}, nowUs);  // the host sees nothing while the menu is up
  switch (state) {
    case MenuState::Open: {
      portENTER_CRITICAL(&statusLock);
      const bool recalibrate = recalibrateRequested;
      recalibrateRequested = false;
      portEXIT_CRITICAL(&statusLock);
      if (recalibrate) InputReader::recalibrate();  // ~12 ms, fine while the host gets a neutral report

      const uint32_t newlyPressed = input.buttons & ~menuPreviousButtons;
      menuPreviousButtons = input.buttons;
      portENTER_CRITICAL(&statusLock);
      menuInput.held = input.buttons;
      menuInput.pressed |= newlyPressed;
      menuInput.leftX = input.axis(Axis::LeftX);
      menuInput.leftY = input.axis(Axis::LeftY);
      portEXIT_CRITICAL(&statusLock);
      break;
    }
    case MenuState::Saved:
      Profiles::replaceAll(menuList.profiles, menuList.count, menuList.active);
      closeMenu();
      break;
    case MenuState::Discarded:
      Profiles::resync();
      closeMenu();
      break;
    case MenuState::Closed: break;
  }
}

// UI task: the inputs since the last call.
ConfigMenu::Input takeMenuInput() {
  portENTER_CRITICAL(&statusLock);
  const ConfigMenu::Input input = menuInput;
  menuInput.pressed = 0;
  portEXIT_CRITICAL(&statusLock);
  return input;
}

// UI task. Slots first, then the count (see Storage.h). Slots flash doesn't hold yet are written too, so the first
// save after running on the presets stores the whole list.
void saveMenuList() {
  const uint8_t stored = Storage::savedProfileCount();
  bool ok = true;
  uint8_t written = 0;
  for (uint8_t i = 0; i < menuList.count; ++i) {
    if ((menuList.changedSlots & (1u << i)) == 0 && i < stored) continue;
    ok = Storage::saveProfile(i, menuList.profiles[i]) && ok;
    ++written;
  }
  ok = Storage::saveProfileCount(menuList.count) && ok;
  if (menuList.displayChanged) {
    ok = Storage::saveDisplaySettings(menuList.display) && ok;
    displaySettings = menuList.display;
  }
  if (menuList.powerChanged) {
    ok = Storage::savePowerSettings(menuList.power) && ok;
    powerSettings = menuList.power;
  }
  LOG("Storage: %u profile(s)%s%s written, %s", written, menuList.displayChanged ? " + display settings" : "",
      menuList.powerChanged ? " + power settings" : "", ok ? "OK" : "FAILED");
}

// UI task. Deep sleep with no wake-up source: the load drops far below the IP5306's threshold, the IP5306 switches
// the 5 V off, and the power switch (off, on) starts the controller again.
[[noreturn]] void powerOff() {
  poweringOff = true;
  vTaskDelay(pdMS_TO_TICKS(Config::INPUT_SCAN_PERIOD_MS * 5));  // any report in flight finishes
  Display::powerOff();
  BleOutput::shutdown();
  vTaskDelay(pdMS_TO_TICKS(Config::POWER_OFF_DISCONNECT_MS));
  LOG("power: deep sleep; toggle the power switch to start again");
  Serial.flush();
  esp_deep_sleep_start();
}

// UI task: the full-screen alert for a PowerManager decision.
void showPowerAlert(const PowerManager::Decision& power) {
  char big[8];
  switch (power.alert) {
    case PowerManager::Alert::AutoOffCountdown:
      snprintf(big, sizeof(big), "%u s", power.secondsLeft);
      Display::showAlert("Powering off in", big, "press any button");
      break;
    case PowerManager::Alert::BatteryCritical:
      snprintf(big, sizeof(big), "%u%%", power.batteryPercent);
      Display::showAlert("Battery critical", big, "charge now");
      break;
    case PowerManager::Alert::BatteryEmpty:
      snprintf(big, sizeof(big), "%u%%", power.batteryPercent);
      Display::showAlert("Battery empty", big, "powering off");
      break;
    case PowerManager::Alert::None: break;
  }
}

#ifdef CONTROLLER_DEBUG
// A power-off that wasn't the switch shows up here (brownout: loose board or weak battery).
const char* resetReasonName(esp_reset_reason_t reason) {
  switch (reason) {
    case ESP_RST_POWERON: return "power-on";
    case ESP_RST_BROWNOUT: return "BROWNOUT (supply voltage dipped)";
    case ESP_RST_SW: return "software restart";
    case ESP_RST_PANIC: return "CRASH (panic)";
    case ESP_RST_INT_WDT:
    case ESP_RST_TASK_WDT:
    case ESP_RST_WDT: return "WATCHDOG";
    case ESP_RST_EXT: return "reset pin";
    default: return "other";
  }
}
#endif

void inputTask(void*) {
  TickType_t lastWake = xTaskGetTickCount();
  uint32_t previousStartUs = micros();
  for (;;) {
    vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(Config::INPUT_SCAN_PERIOD_MS));
    const uint32_t startUs = micros();
    const float dtS = (startUs - previousStartUs) * 1e-6f;
    previousStartUs = startUs;
    if (poweringOff) continue;  // the UI task is shutting the controller down

    const InputState& input = InputReader::read(startUs);
    if (isActivity(input)) lastInputMs = millis();

    const MenuState menu = readMenuState();
    if (menu != MenuState::Closed) {
      menuCycle(menu, input, startUs);
    } else {
      uint32_t suppressedButtons = 0;
      const ProfileButton::Event gesture = ProfileButton::update(input, millis(), suppressedButtons);
      if (gesture.gesture == ProfileButton::Gesture::OpenMenu) openMenu(input);
      handleProfileGesture(gesture);
      InputState forHost = input;
      forHost.buttons &= ~suppressedButtons;
      BleOutput::send(Profiles::apply(forHost, startUs, dtS), startUs);
    }
    publishStatus();
  }
}

void uiTask(void*) {
  // Started here rather than in setup() so the I2C interrupts are allocated on this core, away from input.
  Wire.begin(Config::I2C_SDA, Config::I2C_SCL, Config::I2C_FREQUENCY_HZ);
  BatteryMonitor::begin();
  Display::begin();
  Display::applySettings(displaySettings);

  uint8_t batteryReportedToHost = UINT8_MAX;
  // The active profile is saved once it has stayed put for a while, so flicking through profiles doesn't write
  // flash on every press (each write briefly stalls both cores).
  uint8_t savedProfileNumber = readStatus().profileNumber;
  uint8_t seenProfileNumber = savedProfileNumber;
  uint32_t profileSeenSinceMs = millis();
  bool menuRunning = false;
  TickType_t lastWake = xTaskGetTickCount();
  for (;;) {
    vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(Config::UI_TASK_PERIOD_MS));
    const uint32_t nowMs = millis();

    const bool newBatteryReading = BatteryMonitor::update(nowMs);
    const BatteryStatus battery = BatteryMonitor::status();
    const bool connected = BleOutput::isConnected();
    // Sent once per connection and on every change; the host reads it when it connects.
    if (!connected) {
      batteryReportedToHost = UINT8_MAX;
    } else if (battery.present && battery.percent != batteryReportedToHost) {
      BleOutput::setBatteryLevel(battery.percent);
      batteryReportedToHost = battery.percent;
    }

    const Status status = readStatus();
    if (status.profileNumber != seenProfileNumber) {
      seenProfileNumber = status.profileNumber;
      profileSeenSinceMs = nowMs;
    }
    if (seenProfileNumber != savedProfileNumber && nowMs - profileSeenSinceMs >= Config::ACTIVE_PROFILE_SAVE_DELAY_MS) {
      Storage::saveActiveIndex(seenProfileNumber - 1);
      savedProfileNumber = seenProfileNumber;
    }

    const PowerManager::Decision power =
        PowerManager::update(nowMs, status.lastInputMs, connected, battery, newBatteryReading, powerSettings);
    if (power.powerOff) powerOff();
    const bool alert = power.alert != PowerManager::Alert::None;

    if (readMenuState() == MenuState::Open) {
      if (!menuRunning) {
        menuList.display = displaySettings;
        menuList.power = powerSettings;
        ConfigMenu::open(menuList, nowMs);
        menuRunning = true;
      }
      ConfigMenu::Input input = takeMenuInput();
      // The press that wakes the screen or cancels an alert does nothing else.
      input.screenWasOff =
          alert || (Display::asleep() && (input.held != 0 || input.pressed != 0 || stickActive(input.leftX, input.leftY)));
      const ConfigMenu::Request request = ConfigMenu::update(input, nowMs);
      // Display settings are previewed while they're edited; Discard puts the saved ones back.
      Display::applySettings(request == ConfigMenu::Request::Discard ? displaySettings : menuList.display);
      if (alert) showPowerAlert(power);
      else Display::showMenu(ConfigMenu::view(), status.lastInputMs, nowMs);  // "Saving..." before the flash writes
      switch (request) {
        case ConfigMenu::Request::Save:
          saveMenuList();
          menuRunning = false;
          setMenuState(MenuState::Saved);
          break;
        case ConfigMenu::Request::Discard:
          menuRunning = false;
          setMenuState(MenuState::Discarded);
          break;
        case ConfigMenu::Request::ForgetPairings: BleOutput::forgetPairings(); break;
        case ConfigMenu::Request::RecalibrateSticks:
          portENTER_CRITICAL(&statusLock);
          recalibrateRequested = true;
          portEXIT_CRITICAL(&statusLock);
          break;
        case ConfigMenu::Request::None: break;
      }
      continue;
    }

    if (alert) {
      showPowerAlert(power);
      continue;
    }

    Display::State screen;
    memcpy(screen.profileName, status.profileName, sizeof(screen.profileName));
    screen.profileNumber = status.profileNumber;
    screen.profileCount = status.profileCount;
    screen.profileSwitches = status.profileSwitches;
    screen.layerActive = status.layerActive;
    screen.toggleActive = status.toggleActive;
    screen.bleConnected = connected;
    screen.battery = battery;
    screen.lastInputMs = status.lastInputMs;
    Display::update(screen, nowMs);
  }
}

}  // namespace

void setup() {
  Serial.setTxBufferSize(1024);
  Serial.begin(115200);
  LOG("reset reason: %s", resetReasonName(esp_reset_reason()));

  // Storage first: the profiles and the last active profile come from it.
  Storage::begin();
  Profiles::begin(Storage::loadProfiles, Storage::loadActiveIndex());
  displaySettings = Storage::loadDisplaySettings();
  powerSettings = Storage::loadPowerSettings();
  InputReader::begin();
  ProfileButton::begin();
  BleOutput::begin();
  PowerManager::begin();
  applyActiveProfileSettings();
  publishStatus();
  // BatteryMonitor and Display start in uiTask, which owns the I2C bus.

  xTaskCreatePinnedToCore(inputTask, "input", Config::INPUT_TASK_STACK_BYTES, nullptr, Config::INPUT_TASK_PRIORITY,
                          nullptr, Config::INPUT_TASK_CORE);
  xTaskCreatePinnedToCore(uiTask, "ui", Config::UI_TASK_STACK_BYTES, nullptr, Config::UI_TASK_PRIORITY, nullptr,
                          Config::UI_TASK_CORE);

  LOG("boot complete");
}

void loop() {
  // All work runs in the tasks created in setup().
  vTaskDelete(nullptr);
}
