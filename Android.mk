LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := image2bmp.c
LOCAL_C_INCLUDES := external/jpeg
LOCAL_SHARED_LIBRARIES := libjpeg

LOCAL_MODULE := libimage2bmp
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := test_jpeg.c
LOCAL_SHARED_LIBRARIES := libimage2bmp

LOCAL_MODULE := test_jpeg
include $(BUILD_EXECUTABLE)

