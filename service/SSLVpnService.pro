# ----------------------------------------------------
# This file is generated by the Qt Visual Studio Add-in.
# ------------------------------------------------------

TEMPLATE = app
TARGET = SSLVpnService
DESTDIR = ../Debug
QT += core sql network xml widgets gui
CONFIG += debug console
CONFIG -= flat
DEFINES += _CONSOLE QT_DLL QT_HAVE_MMX QT_HAVE_3DNOW QT_HAVE_SSE QT_HAVE_MMXEXT QT_HAVE_SSE2
INCLUDEPATH += ./debug \
    $(QTDIR)/mkspecs/win32-msvc2012 \
    ./src
LIBS += -luser32
DEPENDPATH += .
MOC_DIR += debug
OBJECTS_DIR += debug
UI_DIR += ./GeneratedFiles
RCC_DIR += ./GeneratedFiles
include(SSLVpnService.pri)
