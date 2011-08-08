include (../liteide.pri)

DESTDIR = $$IDE_LIB_PATH
LIBS += -L$$DESTDIR

INCLUDEPATH += $$IDE_SOURCE_TREE/src/api

isEmpty(TARGET) {
    error("liteideapi.pri: You must provide a TARGET")
}

contains(QT_CONFIG, reduce_exports):CONFIG += hide_symbols

!macx {
    target.path = /lib
    INSTALLS += target
}

TARGET = $$qtLibraryTarget($$TARGET)

