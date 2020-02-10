TEMPLATE = app
TARGET = Audio_Recorder
DESTDIR = ./bin/Release
CONFIG += release
LIBS += -L"."
DEPENDPATH += .
MOC_DIR += .
OBJECTS_DIR += release
UI_DIR += .
RCC_DIR += .
include(vendor/vendor.pri)
include(Audio recorder.pri)
unix:RC_FILE = Audio recorder.rc
