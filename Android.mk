LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	AudioRecordTest.cpp

# android 7.1
LOCAL_SHARED_LIBRARIES := \
	libutils libcutils liblog \
 	libmedia \

# android 10
# LOCAL_SHARED_LIBRARIES := \
 	libutils libcutils liblog \
  	libmedia \
 	libtinyalsa libaudioclient 

LOCAL_MODULE:= AudioRecordTest

LOCAL_MODULE_TAGS := tests

include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	pcm2wav.cpp

LOCAL_SHARED_LIBRARIES := \
	libcutils \
	libutils \
    libmedia

# LOCAL_CFLAGS += -Wno-multichar -lrt -std=c++11

LOCAL_MODULE:= pcm2wav

LOCAL_MODULE_TAGS := tests

include $(BUILD_EXECUTABLE)
