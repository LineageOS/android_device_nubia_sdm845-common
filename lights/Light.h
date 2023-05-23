/*
 * Copyright (C) 2023 The LineageOS Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <aidl/android/hardware/light/BnLights.h>
#include <android-base/logging.h>
#include <android-base/properties.h>
#include <hardware/hardware.h>
#include <hardware/lights.h>
#include <vector>

using ::aidl::android::hardware::light::HwLightState;
using ::aidl::android::hardware::light::HwLight;
using ::aidl::android::hardware::light::LightType;
using ::aidl::android::hardware::light::BnLights;
using android::base::GetProperty;

static unsigned int brightness_table[256] = {
    0,    1,    17,   65,   97,   146,  178,  226,  436,  597,  758,  887,
    968,  1080, 1161, 1258, 1338, 1371, 1451, 1532, 1580, 1661, 1709, 1741,
    1822, 1870, 1902, 1983, 2031, 2080, 2112, 2160, 2193, 2241, 2273, 2322,
    2322, 2322, 2322, 2450, 2450, 2483, 2531, 2531, 2563, 2563, 2612, 2612,
    2644, 2644, 2692, 2692, 2724, 2724, 2773, 2773, 2805, 2805, 2853, 2853,
    2853, 2902, 2902, 2902, 2934, 2934, 2934, 2982, 2982, 2982, 3015, 3015,
    3015, 3063, 3063, 3063, 3063, 3095, 3095, 3095, 3095, 3144, 3144, 3176,
    3176, 3176, 3176, 3176, 3224, 3224, 3224, 3272, 3272, 3272, 3272, 3272,
    3305, 3305, 3305, 3305, 3305, 3353, 3353, 3353, 3353, 3353, 3385, 3385,
    3385, 3385, 3385, 3434, 3434, 3434, 3434, 3434, 3466, 3466, 3466, 3466,
    3466, 3466, 3466, 3466, 3514, 3514, 3514, 3514, 3546, 3546, 3546, 3546,
    3546, 3546, 3595, 3595, 3595, 3595, 3595, 3595, 3595, 3595, 3595, 3595,
    3627, 3627, 3627, 3627, 3627, 3627, 3627, 3675, 3675, 3675, 3675, 3675,
    3675, 3675, 3724, 3724, 3724, 3724, 3724, 3724, 3724, 3724, 3724, 3724,
    3756, 3756, 3756, 3756, 3756, 3756, 3756, 3756, 3756, 3804, 3804, 3804,
    3804, 3804, 3804, 3804, 3804, 3804, 3837, 3837, 3837, 3837, 3837, 3837,
    3837, 3837, 3837, 3837, 3885, 3885, 3885, 3885, 3885, 3885, 3885, 3885,
    3885, 3885, 3917, 3917, 3917, 3917, 3917, 3917, 3917, 3917, 3917, 3917,
    3917, 3917, 3966, 3966, 3966, 3966, 3966, 3966, 3966, 3966, 3966, 3966,
    3966, 3998, 3998, 3998, 3998, 3998, 3998, 3998, 3998, 3998, 3998, 3998,
    3998, 4046, 4046, 4046, 4046, 4046, 4046, 4046, 4046, 4046, 4046, 4046,
    4046, 4046, 4095, 4095
};

namespace aidl {
namespace android {
namespace hardware {
namespace light {

class Lights : public BnLights {
      ndk::ScopedAStatus setLightState(int id, const HwLightState& state) override;
      ndk::ScopedAStatus getLights(std::vector<HwLight>* types) override;
};

}  // namespace light
}  // namespace hardware
}  // namespace android
}  // namespace aidl
