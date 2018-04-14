LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := android.hardware.power@1.3-service.nubia_sdm845-libperfmgr
LOCAL_INIT_RC := android.hardware.power@1.3-service.nubia_sdm845-libperfmgr.rc
LOCAL_VINTF_FRAGMENTS := android.hardware.power@1.3-service.nubia_sdm845-libperfmgr.xml
LOCAL_VENDOR_MODULE := true
LOCAL_MODULE_RELATIVE_PATH := hw

LOCAL_SRC_FILES := \
    service.cpp \
    Power.cpp \
    InteractionHandler.cpp \
    power-helper.c

LOCAL_SHARED_LIBRARIES := \
    libbase \
    libhidlbase \
    liblog \
    libutils \
    libcutils \
    android.hardware.power@1.0 \
    android.hardware.power@1.1 \
    android.hardware.power@1.2 \
    android.hardware.power@1.3 \
    libperfmgr

LOCAL_HEADER_LIBRARIES := \
    libhardware_headers

LOCAL_CFLAGS := -Wall -Werror

ifneq ($(TARGET_TAP_TO_WAKE_NODE),)
    LOCAL_CFLAGS += -DTAP_TO_WAKE_NODE=\"$(TARGET_TAP_TO_WAKE_NODE)\"
endif

include $(BUILD_EXECUTABLE)

include $(call all-makefiles-under,$(LOCAL_PATH))
