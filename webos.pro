TARGET = qwebos

QT += \
    core-private gui-private \
    eventdispatcher_support-private \
    service_support-private theme_support-private fontdatabase_support-private

qtHaveModule(platformcompositor_support-private): \
    QT += platformcompositor_support-private

# Avoid X11 header collision, use generic EGL native types
DEFINES += QT_EGL_NO_X11

SOURCES += $$PWD/qwebosmain.cpp
include($$PWD/api/api.pri)

QMAKE_LFLAGS += $$QMAKE_LFLAGS_NOUNDEF
LIBS += -lSDL -lpdl

OTHER_FILES += $$PWD/webos.json

PLUGIN_TYPE = platforms
PLUGIN_CLASS_NAME = QWebOSIntegrationPlugin
!equals(TARGET, $$QT_DEFAULT_QPA_PLUGIN): PLUGIN_EXTENDS = -
load(qt_plugin)
