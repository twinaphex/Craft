LOCAL_PATH := $(call my-dir)

ROOT_DIR := $(LOCAL_PATH)/..

HAVE_OPENGL := 1
GLES        := 1
SOURCES_C   :=
INCFLAGS    :=
GIT_VERSION :=

include $(ROOT_DIR)/Makefile.common

COREFLAGS := -D__LIBRETRO__ -DINLINE="inline" -DANDROID $(CFLAGS) $(GLFLAGS) $(INCFLAGS)

include $(CLEAR_VARS)
LOCAL_MODULE    := retro
LOCAL_SRC_FILES := $(SOURCES_C)
LOCAL_CFLAGS    := $(COREFLAGS) -std=gnu99
LOCAL_LDFLAGS   := -Wl,-version-script=$(ROOT_DIR)/link.T
LOCAL_LDLIBS    := -lGLESv2

ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
  LOCAL_ARM_NEON := true
endif

include $(BUILD_SHARED_LIBRARY)
