TEMPLATE = lib 
TARGET = $$qtLibraryTarget(meego-keyboard)

OBJECTS_DIR = .obj
MOC_DIR = .moc

# we have this line temporarily until new libmeegotouch without rpath is integrated
QT += xml

contains( DEFINES, RELEASE_BUILD ) {
    QMAKE_CFLAGS -= -Werror
    QMAKE_CXXFLAGS -= -Werror
} else {
    QMAKE_CFLAGS += -Werror
    QMAKE_CXXFLAGS += -Werror
} 

CONFIG += plugin meegotouch meegoimengine meegoimenginewords meegoimframework meegoreactionmap
#CONFIG += mcontrolpanel
DEFINES += NOCONTROLPANEL

CONFIG += link_pkgconfig
PKGCONFIG += gconf-2.0 xkbfile

# coverage flags are off per default, but can be turned on via qmake COV_OPTION=on
for(OPTION,$$list($$lower($$COV_OPTION))){
    isEqual(OPTION, on){
        QMAKE_CXXFLAGS += -ftest-coverage -fprofile-arcs -fno-elide-constructors
        LIBS += -lgcov
        CONFIG -= release
        CONFIG += debug
    }
}

QMAKE_CLEAN += *.gcno *.gcda

include(common/common.pri)
include(widgets/widgets.pri)
include(theme/theme.pri)
include(layouts/layouts.pri)

HEADERS += \
    mkeyboardhost.h \
    mkeyboardsettings.h \
    mkeyboardplugin.h \
    mkeyboardhost_p.h \

SOURCES += \
    mkeyboardhost.cpp \
    mkeyboardsettings.cpp \
    mkeyboardplugin.cpp \

target.path += /usr/lib/meego-im-plugins

install_headers.path = /usr/include/meego-keyboard
install_headers.files = $$INSTALL_HEADERS


INSTALLS += \
    target \
    install_headers \

QMAKE_EXTRA_TARGETS += check-xml
check-xml.depends = lib$${TARGET}.so
check-xml.commands = $$system(true)

QMAKE_EXTRA_TARGETS += check
check.depends = lib$${TARGET}.so
check.commands = $$system(true)

