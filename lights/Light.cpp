/*
 * Copyright (C) 2023 The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include "Light.h"

#include <fstream>

#define LCD_LED           "/sys/class/backlight/panel0-backlight/brightness"

#define NUBIA_LED_MODE    "/sys/class/leds/nubia_led/blink_mode"
#define NUBIA_LED_COLOR   "/sys/class/leds/nubia_led/outn"
#define NUBIA_GRADE       "/sys/class/leds/nubia_led/grade_parameter"
#define NUBIA_FADE        "/sys/class/leds/nubia_led/fade_parameter"

#define BATTERY_STATUS_FILE       "/sys/class/power_supply/battery/status"
#define BATTERY_CAPACITY          "/sys/class/power_supply/battery/capacity"

#define BATTERY_STATUS_FULL         "Full"
#define BATTERY_STATUS_DISCHARGING  "Discharging"
#define BATTERY_STATUS_CHARGING     "Charging"

#define BLINK_MODE_ON    3
#define BLINK_MODE_CONST 1
#define BLINK_MODE_OFF   0

#define NUBIA_LED_DISABLE 16
#define NUBIA_LED_RED     48
#define NUBIA_LED_GREEN   64

#define BREATH_SOURCE_NONE		0x00
#define BREATH_SOURCE_NOTIFICATION	0x01
#define BREATH_SOURCE_BATTERY		0x02
#define BREATH_SOURCE_BUTTONS		0x04
#define BREATH_SOURCE_ATTENTION		0x08

#define MAX_LED_BRIGHTNESS    255
#define MAX_LCD_BRIGHTNESS    4095

#define BACK_LED_EFFECT_FILE       "/sys/class/leds/aw22xxx_led/effect"

#define BACK_LED_OFF               0
#define BACK_LED_NOTIFICATION      2
#define BACK_LED_BATTERY_CHARGING  8
#define BACK_LED_BATTERY_FULL      11
#define BACK_LED_BATTERY_LOW       33

static int32_t active_status = 0;

enum battery_status {
    BATTERY_UNKNOWN = 0,
    BATTERY_LOW,
    BATTERY_FREE,
    BATTERY_CHARGING,
    BATTERY_FULL,
};

namespace {
/*
 * Write value to path and close file.
 */
static void set(std::string path, std::string value) {
    std::ofstream file(path);

    if (!file.is_open()) {
        LOG(WARNING) << "failed to write " << value.c_str() << " to " << path.c_str();
        return;
    }

    file << value;
}

static int get(std::string path) {
    std::ifstream file(path);
    int value;

    if (!file.is_open()) {
        LOG(WARNING) << "failed to read from: " << path.c_str();
        return 0;
    }

    file >> value;
    return value;
}

static int readStr(std::string path, char *buffer, size_t size)
{

    std::ifstream file(path);

    if (!file.is_open()) {
        LOG(WARNING) << "failed to read: " << path.c_str();
        return -1;
    }

    file.read(buffer, size);
    file.close();
    return 1;
}

static void set(std::string path, int value) {
    set(path, std::to_string(value));
}

static uint32_t getBrightness(const HwLightState& state) {
    uint32_t alpha, red, green, blue;

    /*
     * Extract brightness from AARRGGBB.
     */
    alpha = (state.color >> 24) & 0xFF;
    red = (state.color >> 16) & 0xFF;
    green = (state.color >> 8) & 0xFF;
    blue = state.color & 0xFF;

    /*
     * Scale RGB brightness if Alpha brightness is not 0xFF.
     */
    if (alpha != 0xFF) {
        red = red * alpha / 0xFF;
        green = green * alpha / 0xFF;
        blue = blue * alpha / 0xFF;
    }

    return (77 * red + 150 * green + 29 * blue) >> 8;
}

int getBatteryStatus()
{
    int err;

    char status_str[16];
    int capacity = 0;

    err = readStr(BATTERY_STATUS_FILE, status_str, sizeof(status_str));
    if (err <= 0) {
        LOG(WARNING) << "failed to read battery status: " << err;
        return BATTERY_UNKNOWN;
    }

    capacity = get(BATTERY_CAPACITY);

    if (0 == strncmp(status_str, BATTERY_STATUS_FULL, 4)) {
            return BATTERY_FULL;
        }

    if (0 == strncmp(status_str, BATTERY_STATUS_DISCHARGING, 11)) {
            return BATTERY_FREE;
        }

    if (0 == strncmp(status_str, BATTERY_STATUS_CHARGING, 8)) {
        if (capacity < 90) {
            return BATTERY_CHARGING;
        } else {
            return BATTERY_FULL;
        }
    } else {
        if (capacity < 10) {
            return BATTERY_LOW;
        } else {
            return BATTERY_FREE;
        }
    }
}

static inline uint32_t scaleBrightness(uint32_t brightness, uint32_t maxBrightness) {
    // Map brightness values logarithmatically to match aosp behaviour
    LOG(DEBUG) << "Received brightness: " << brightness;
    if (maxBrightness == MAX_LCD_BRIGHTNESS)
        return brightness_table[brightness];
    return brightness;
}

static inline uint32_t getScaledBrightness(const HwLightState& state, uint32_t maxBrightness) {
    return scaleBrightness(getBrightness(state), maxBrightness);
}

static void handleBacklight(const HwLightState& state) {
    uint32_t brightness = getScaledBrightness(state, MAX_LCD_BRIGHTNESS);
    LOG(DEBUG) << "Setting brightness: " << brightness;
    set(LCD_LED, brightness);
}


/*
 * Set the the LED color and blinking mode for battery states.
 */
static void setBatteryBreathLight() {
    if (active_status == BREATH_SOURCE_BATTERY) {
        int battery_state = getBatteryStatus();

	if(battery_state == BATTERY_CHARGING) {
            LOG(DEBUG) << "BATTERY CHARGING";
            // Charging -- Set top led light up (red)
            set(NUBIA_LED_COLOR, NUBIA_LED_RED);
            set(NUBIA_FADE, "0 0 0");
            set(NUBIA_GRADE, "100 255");
            set(NUBIA_LED_MODE, BLINK_MODE_CONST);
            if (GetProperty("ro.product.vendor.device", "") == "NX619J") {
                // Set back led strip scrolling (green)
                set(BACK_LED_EFFECT_FILE, BACK_LED_BATTERY_CHARGING);
            }
	}else if (battery_state == BATTERY_LOW) {
            LOG(DEBUG) << "BATTERY LOW";
            // Low -- Set top led blink (red)
            set(NUBIA_LED_COLOR, NUBIA_LED_RED);
            set(NUBIA_FADE, "3 0 4");
            set(NUBIA_GRADE, "0 100");
            set(NUBIA_LED_MODE, BLINK_MODE_ON);
            if (GetProperty("ro.product.vendor.device", "") == "NX619J") {
                // Set back led strip blink(red)
                set(BACK_LED_EFFECT_FILE, BACK_LED_BATTERY_LOW);
            }
	}else if (battery_state == BATTERY_FULL) {
            LOG(DEBUG) << "BATTERY FULL";
            if (GetProperty("ro.product.vendor.device", "") == "NX606J") {
                // Full -- Set top led light up (RED)
                set(NUBIA_LED_COLOR, NUBIA_LED_RED);
            } else {
                // Full -- Set top led light up (green)
                set(NUBIA_LED_COLOR, NUBIA_LED_GREEN);
            }
            set(NUBIA_FADE, "0 0 0");
            set(NUBIA_GRADE, "100 255");
            set(NUBIA_LED_MODE, BLINK_MODE_CONST);
            if (GetProperty("ro.product.vendor.device", "") == "NX619J") {
                // Set back led strip scrolling (rainbow)
                set(BACK_LED_EFFECT_FILE, BACK_LED_BATTERY_FULL);
            }
	} else if (battery_state == BATTERY_FREE) {
            LOG(DEBUG) << "BATTERY FREE OR DISCHARGING";
            // Disable blinking to start. Turn off all colors of led
            set(NUBIA_LED_MODE, BLINK_MODE_OFF);
            if (GetProperty("ro.product.vendor.device", "") == "NX619J") {
                // turn off back led strip
                set(BACK_LED_EFFECT_FILE, BACK_LED_OFF);
            }
	}
    }
}

/*
 * Set the the LED color and blinking mode for notification breath light.
 */
static void setNotificationBreathLight() {
    if (GetProperty("ro.product.vendor.device", "") == "NX606J") {
        set(NUBIA_LED_COLOR, NUBIA_LED_RED);
    } else {
        set(NUBIA_LED_COLOR, NUBIA_LED_GREEN);
    }
    set(NUBIA_FADE, "3 0 4");
    set(NUBIA_GRADE, "0 100");
    set(NUBIA_LED_MODE, BLINK_MODE_ON);
    if (GetProperty("ro.product.vendor.device", "") == "NX619J") {
        // Set back led strip breath (green)
        set(BACK_LED_EFFECT_FILE, BREATH_SOURCE_NOTIFICATION);
    }
}

/*
 * Set the LED color, blinking mode for breath light, and determine the primary event
 */
static uint32_t setBreathLightLocked(uint32_t event_source, const HwLightState& state){
    uint32_t brightness = getScaledBrightness(state, MAX_LED_BRIGHTNESS);

    if (brightness > 0) {
        active_status |= event_source;
    } else {
        active_status &= ~event_source;
    }

    if(active_status == 0) { //nothing, close all
        // disable green led
        set(NUBIA_LED_COLOR, NUBIA_LED_GREEN);
        set(NUBIA_LED_MODE, BLINK_MODE_OFF);
        // disable red led
        set(NUBIA_LED_COLOR, NUBIA_LED_RED);
        set(NUBIA_LED_MODE, BLINK_MODE_OFF);
        // set disable led
        set(NUBIA_LED_COLOR, NUBIA_LED_DISABLE);
        set(NUBIA_LED_MODE, BLINK_MODE_OFF);
        set(NUBIA_FADE, "0 0 0");
        set(NUBIA_GRADE, "100 255");
        if (GetProperty("ro.product.vendor.device", "") == "NX619J") {
            // turn off back led strip
            set(BACK_LED_EFFECT_FILE, BACK_LED_OFF);
        }
        return 0;
    }

    uint32_t saved_status = 0;
    if (active_status & BREATH_SOURCE_BATTERY) {
        if ((active_status & BREATH_SOURCE_NOTIFICATION) || (active_status & BREATH_SOURCE_ATTENTION)) {
            // pause BREATH_SOURCE_BATTERY event and save its status
            saved_status = active_status;
            active_status &= ~BREATH_SOURCE_BATTERY;
        } else {
            // BREATH_SOURCE_BATTERY is the primary event
            setBatteryBreathLight();
            return 0;
        }
    }

    if (active_status & BREATH_SOURCE_NOTIFICATION || active_status & BREATH_SOURCE_ATTENTION) {
        setNotificationBreathLight();
    }

    if (saved_status & BREATH_SOURCE_BATTERY) {
        active_status |= BREATH_SOURCE_BATTERY;
        saved_status &= ~BREATH_SOURCE_BATTERY;
        if (saved_status != 0) {
            // resume saved event
            if (saved_status & BREATH_SOURCE_NOTIFICATION || saved_status & BREATH_SOURCE_ATTENTION) {
                setNotificationBreathLight();
            }
        }
    }
    return 0;
}

static inline bool isLit(const HwLightState& state) {
    return state.color & 0x00ffffff;
}

static void handleNotification(const HwLightState& state) {
    setBreathLightLocked(BREATH_SOURCE_NOTIFICATION, state);
}

static void handleBattery(const HwLightState& state){
    setBreathLightLocked(BREATH_SOURCE_BATTERY, state);
}

/* Keep sorted in the order of importance. */
static std::vector<LightType> backends = {
    LightType::ATTENTION,
    LightType::BACKLIGHT,
    LightType::BATTERY,
    LightType::NOTIFICATIONS,
};

}  // anonymous namespace

namespace aidl {
namespace android{
namespace hardware {
namespace light {

ndk::ScopedAStatus Lights::setLightState(int id, const HwLightState& state) {
    switch(id) {
        case (int) LightType::ATTENTION:
            handleNotification(state);
            return ndk::ScopedAStatus::ok();
        case (int) LightType::BACKLIGHT:
            handleBacklight(state);
            return ndk::ScopedAStatus::ok();
        case (int) LightType::BATTERY:
            handleBattery(state);
            return ndk::ScopedAStatus::ok();
        case (int) LightType::NOTIFICATIONS:
            handleNotification(state);
            return ndk::ScopedAStatus::ok();
        default:
            return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
    }
}

ndk::ScopedAStatus Lights::getLights(std::vector<HwLight>* lights) {
    int i = 0;

    for (const LightType& backend : backends) {
        HwLight hwLight;
        hwLight.id = (int) backend;
        hwLight.type = backend;
        hwLight.ordinal = i;
        lights->push_back(hwLight);
        i++;
    }

    return ndk::ScopedAStatus::ok();
}

}  // namespace light
}  // namespace hardware
}  // namespace android
}  // namespace aidl
