#-------------------------------------------------
#
# Project created by QtCreator 2015-02-25T18:16:55
#
#-------------------------------------------------

QT	   += network xml core gui widgets
u
TARGET = safechecker
TEMPLATE = app
INCLUDEPATH += ../safejumper/ \
               ../common/

DEFINES += SAFECHECKER

macx: {
    TARGET = Safechecker
    QMAKE_INFO_PLIST = ./Info.plist
    QMAKE_LFLAGS += -F /System/Library/Frameworks/
    QMAKE_RPATHDIR += @executable_path/../Frameworks
    LIBS += -framework Security
    target.path = /Applications
    resources.path = /Applications/Safechecker.app/Contents/Resources
    resources.files = ./resources/*
    INSTALLS = target resources
    ICON = ./safechecker.icns
}

win32: {
    WINSDK_DIR = C:/Program Files/Microsoft SDKs/Windows/v7.1
    WIN_PWD = $$replace(PWD, /, \\)
    OUT_PWD_WIN = $$replace(OUT_PWD, /, \\)
    QMAKE_POST_LINK = "$$quote($$OUT_PWD_WIN\\..\\fixbinary.bat) $$quote($$OUT_PWD_WIN\\..\\safechecker.exe) $$quote($$WIN_PWD\\..\\safejumper\\safejumper.manifest)"
    RC_FILE = safechecker.rc
    LIBS += -lws2_32 -lIphlpapi
    DESTDIR = ../../buildwindows/
    MOC_DIR = ../.obj/
    HEADERS += \
        ../safejumper/qtlocalpeer.h \
        ../safejumper/qtlockedfile.h \
        ../safejumper/qtsingleapplication.h \

    SOURCES += \
        ../safejumper/qtlocalpeer.cpp \
        ../safejumper/qtlockedfile.cpp \
        ../safejumper/qtsingleapplication.cpp \
}

linux: {
    HEADERS += \
        ../safejumper/qtlocalpeer.h \
        ../safejumper/qtlockedfile.h \
        ../safejumper/qtsingleapplication.h \

    SOURCES += \
        ../safejumper/qtlocalpeer.cpp \
        ../safejumper/qtlockedfile.cpp \
        ../safejumper/qtsingleapplication.cpp \

    CONFIG += static
}

SOURCES += \
    ../safejumper/authmanager.cpp \
    ../common/common.cpp \
    ../safejumper/confirmationdialog.cpp \
    ../safejumper/dlg_newnode.cpp \
    ../safejumper/encryptiondelegate.cpp \
    ../safejumper/errordialog.cpp \
    ../safejumper/flag.cpp \
    ../safejumper/fonthelper.cpp \
    ../safejumper/locationdelegate.cpp \
    ../common/log.cpp \
    ../safejumper/loginwindow.cpp \
    main.cpp \
    ../safejumper/mapscreen.cpp \
    ../safejumper/ministun.c \
    ../common/osspecific.cpp \
    pathhelper.cpp \
    ../safejumper/pingwaiter.cpp \
    ../safejumper/portforwarder.cpp \
    ../safejumper/protocoldelegate.cpp \
    scr_logs.cpp \
    ../safejumper/setting.cpp \
    ../safejumper/stun.cpp \
    testdialog.cpp \
    ../safejumper/trayiconmanager.cpp \
    wndmanager.cpp \
    settingsscreen.cpp

HEADERS += \
    ../safejumper/authmanager.h \
    ../common/common.h \
    ../safejumper/confirmationdialog.h \
    ../safejumper/dlg_newnode.h \
    ../safejumper/encryptiondelegate.h \
    ../safejumper/errordialog.h \
    ../safejumper/flag.h \
    ../safejumper/fonthelper.h \
    ../safejumper/locationdelegate.h \
    ../common/log.h \
    ../safejumper/loginwindow.h \
    ../safejumper/mapscreen.h \
    ../safejumper/ministun.h \
    ../common/osspecific.h \
    pathhelper.h \
    ../safejumper/pingwaiter.h \
    ../safejumper/portforwarder.h \
    ../safejumper/protocoldelegate.h \
    scr_logs.h \
    ../safejumper/setting.h \
    ../safejumper/stun.h \
    testdialog.h \
    ../safejumper/trayiconmanager.h \
    update.h \
    ../safejumper/version.h \
    wndmanager.h \
    settingsscreen.h

FORMS += \
    ../safejumper/confirmationdialog.ui \
    ../safejumper/dlg_newnode.ui \
    ../safejumper/errordialog.ui \
    ../safejumper/loginwindow.ui \
    ../safejumper/mapscreen.ui \
    scr_logs.ui \
    testdialog.ui \
    settingsscreen.ui

RESOURCES += \
    ../safejumper/imgs.qrc

