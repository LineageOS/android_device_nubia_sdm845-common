/*
 * Copyright (C) 2018-2019 The LineageOS Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "LightsService"

#include "Light.h"

#include <android-base/logging.h>
#include <android-base/stringprintf.h>
#include <fstream>

namespace android {
namespace hardware {
namespace light {
namespace V2_0 {
namespace implementation {

#define MODE_OFF           2
#define MODE_CONSTANT_ON   1
#define MODE_AUTO_BLINK    3

#define LED_CHANNEL_RED      48
#define LED_CHANNEL_GREEN    64
#define LED_CHANNEL_BLUE     80

#define BACK_LED_EFFECT_FILE       "/sys/class/leds/aw22xxx_led/effect"

#define BACK_LED_OFF               0
#define BACK_LED_NOTIFICATION      2
#define BACK_LED_BATTERY_CHARGING  8
#define BACK_LED_BATTERY_FULL      11
#define BACK_LED_BATTERY_LOW       33

/*
 * Write value to path and close file.
 */
template <typename T>
static void set(const std::string& path, const T& value) {
    std::ofstream file(path);
    file << value;
}

/*
 * Read from path and close file.
 * Return def in case of any failure.
 */
template <typename T>
static T get(const std::string& path, const T& def) {
    std::ifstream file(path);
    T result;

    file >> result;
    return file.fail() ? def : result;
}

static constexpr int kDefaultMaxBrightness = 255;

static uint32_t getBrightness(const LightState& state) {
    uint32_t alpha, red, green, blue;

    // Extract brightness from AARRGGBB
    alpha = (state.color >> 24) & 0xff;

    // Retrieve each of the RGB colors
    red = (state.color >> 16) & 0xff;
    green = (state.color >> 8) & 0xff;
    blue = state.color & 0xff;

    // Scale RGB colors if a brightness has been applied by the user
    if (alpha != 0xff) {
        red = red * alpha / 0xff;
        green = green * alpha / 0xff;
        blue = blue * alpha / 0xff;
    }

    return (77 * red + 150 * green + 29 * blue) >> 8;
}

static uint32_t rgbToBrightness(const LightState& state) {
    uint32_t color = state.color & 0x00ffffff;
    return ((77 * ((color >> 16) & 0xff))
            + (150 * ((color >> 8) & 0xff))
            + (29 * (color & 0xff))) >> 8;
}

Light::Light() {
    mLights.emplace(Type::ATTENTION, std::bind(&Light::handleNotification, this, std::placeholders::_1, 0));
    mLights.emplace(Type::BACKLIGHT, std::bind(&Light::handleBacklight, this, std::placeholders::_1));
    mLights.emplace(Type::BATTERY, std::bind(&Light::handleNotification, this, std::placeholders::_1, 1));
    mLights.emplace(Type::NOTIFICATIONS, std::bind(&Light::handleNotification, this, std::placeholders::_1, 2));
}

void Light::handleBacklight(const LightState& state) {
    int maxBrightness = get("/sys/class/backlight/panel0-backlight/max_brightness", -1);
    if (maxBrightness < 0) {
        maxBrightness = kDefaultMaxBrightness;
    }
    uint32_t sentBrightness = rgbToBrightness(state);
    uint32_t brightness = sentBrightness * maxBrightness / kDefaultMaxBrightness;
    LOG(DEBUG) << "Writing backlight brightness " << brightness
               << " (orig " << sentBrightness << ")";
    set("/sys/class/backlight/panel0-backlight/brightness", brightness);
}

void Light::handleNotification(const LightState& state, size_t index) {
    mLightStates.at(index) = state;

    LightState stateToUse = mLightStates.front();
    for (const auto& lightState : mLightStates) {
        if (lightState.color & 0xffffff) {
            stateToUse = lightState;
            break;
        }
    }

    uint32_t brightness = getBrightness(stateToUse);

    uint32_t onMs = stateToUse.flashMode == Flash::TIMED ? stateToUse.flashOnMs : 0;
    uint32_t offMs = stateToUse.flashMode == Flash::TIMED ? stateToUse.flashOffMs : 0;


    // Disable blinking to start. Turn off all colors of led
    set("/sys/class/leds/nubia_led/outn", LED_CHANNEL_RED);
    set("/sys/class/leds/nubia_led/blink_mode", MODE_OFF);
    set("/sys/class/leds/nubia_led/outn", LED_CHANNEL_GREEN);
    set("/sys/class/leds/nubia_led/blink_mode", MODE_OFF);
    set("/sys/class/leds/nubia_led/outn", LED_CHANNEL_BLUE);
    set("/sys/class/leds/nubia_led/blink_mode", MODE_OFF);
    // turn off back led strip
    set(BACK_LED_EFFECT_FILE, BACK_LED_OFF);
    LOG(DEBUG) << "Disable blink ";
    if (brightness <= 0)
    {
        return;
    }

    if (onMs > 0 && offMs > 0) {
        // Turn Of Red Led
        set("/sys/class/leds/nubia_led/outn", LED_CHANNEL_RED);
        set("/sys/class/leds/nubia_led/blink_mode", MODE_AUTO_BLINK);

        // Turn Of Green Led, it is yellow now
        set("/sys/class/leds/nubia_led/outn", LED_CHANNEL_GREEN);
        set("/sys/class/leds/nubia_led/blink_mode", MODE_AUTO_BLINK);

        set("/sys/class/leds/nubia_led/fade_parameter", "3 0 4");
        set("/sys/class/leds/nubia_led/grade_parameter", "10 100");
        // Start blinking
        set("/sys/class/leds/nubia_led/blink_mode", MODE_AUTO_BLINK);
	// Set back led strip breath (green)
        set(BACK_LED_EFFECT_FILE, BACK_LED_NOTIFICATION);
    } else {
        uint32_t capacity = get("/sys/class/power_supply/battery/capacity", 0);
        std::string defualt = "Discharging";
        std::string status = get("/sys/class/power_supply/battery/status", defualt);
        if (capacity >= 90 || status == "FULL")
        {
            set("/sys/class/leds/nubia_led/outn", LED_CHANNEL_GREEN);
            // Set back led strip scrolling (rainbow)
            set(BACK_LED_EFFECT_FILE, BACK_LED_BATTERY_FULL);
        }else if (10 <= capacity < 90 || status == "CHARGING")
        {
            set("/sys/class/leds/nubia_led/outn", LED_CHANNEL_RED);
            set("/sys/class/leds/nubia_led/blink_mode", MODE_CONSTANT_ON);
            set("/sys/class/leds/nubia_led/outn", LED_CHANNEL_GREEN);
            // Set back led strip scrolling (green)
            set(BACK_LED_EFFECT_FILE, BACK_LED_BATTERY_CHARGING);
        }else if (0 < capacity <= 10 || status == "LOW") {
            set("/sys/class/leds/nubia_led/outn", LED_CHANNEL_RED);
            // Set back led strip blink(red)
            set(BACK_LED_EFFECT_FILE, BACK_LED_BATTERY_LOW);
        }
        set("/sys/class/leds/nubia_led/fade_parameter", "0 1 1");
        set("/sys/class/leds/nubia_led/grade_parameter", "20 200");
        set("/sys/class/leds/nubia_led/blink_mode", MODE_CONSTANT_ON);
        LOG(DEBUG) << "battery_status " << status
               << " (capacity " << capacity << ")";
    }
    LOG(DEBUG) << base::StringPrintf(
        "handleRgb: mode=%d, color=%08X, onMs=%d, offMs=%d",
        static_cast<std::underlying_type<Flash>::type>(stateToUse.flashMode), stateToUse.color,
        onMs, offMs);
}

Return<Status> Light::setLight(Type type, const LightState& state) {
    auto it = mLights.find(type);

    if (it == mLights.end()) {
        return Status::LIGHT_NOT_SUPPORTED;
    }

    // Lock global mutex until light state is updated.
    std::lock_guard<std::mutex> lock(mLock);

    it->second(state);

    return Status::SUCCESS;
}

Return<void> Light::getSupportedTypes(getSupportedTypes_cb _hidl_cb) {
    std::vector<Type> types;

    for (auto const& light : mLights) {
        types.push_back(light.first);
    }

    _hidl_cb(types);

    return Void();
}

}  // namespace implementation
}  // namespace V2_0
}  // namespace light
}  // namespace hardware
}  // namespace android
