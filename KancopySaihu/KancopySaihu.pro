# ----------------------------------------------------
# This file is generated by the Qt Visual Studio Tools.
# ------------------------------------------------------

TEMPLATE = app
TARGET = KancopySaihu
DESTDIR = ../x64/Debug
QT += core gui widgets
CONFIG += debug
DEFINES += _UNICODE WIN64 QT_DLL QT_WIDGETS_LIB
INCLUDEPATH += ../../../../portaudio/include \
    ../../../../fftw \
    ../../../../julius/julius_source/libsent/include \
    ../../../../julius/julius_source/libjulius/include \
    ../../../../LibUTAU \
    ./GeneratedFiles \
    . \
    ./GeneratedFiles/$(ConfigurationName)
LIBS += -llibjuliusd \
    -llibsentd \
    -lws2_32 \
    -lzlib_D \
    -lfftw3_x64d \
    -lPortAudio_D \
    -lLibUTAUx64D
DEPENDPATH += .
MOC_DIR += ./GeneratedFiles/$(ConfigurationName)
OBJECTS_DIR += debug
UI_DIR += ./GeneratedFiles
RCC_DIR += ./GeneratedFiles
include(KancopySaihu.pri)
