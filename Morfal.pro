QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += \
    c++2a \
    app_bundle

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    JsonRetrievers/JsonArrayRetriever.cpp \
    JsonRetrievers/JsonConditionRetriever.cpp \
    JsonRetrievers/JsonMapRetriever.cpp \
    JsonRetrievers/JsonRetriever.cpp \
    JsonRetrievers/JsonRetrieverHelper.cpp \
    JsonRetrievers/JsonStringRetriever.cpp \
    Section.cpp \
    charactersheet.cpp \
    dicehelper.cpp \
    drawablemap.cpp \
    fighttracker.cpp \
    interactivemap.cpp \
    jsonfilters/jsonfilter.cpp \
    jsontoqtxmbuilder.cpp \
    main.cpp \
    mainwindow.cpp \
    mapzonegraphicsobject.cpp \
    minimalscopeprofiler.cpp \
    monsterdelegate.cpp \
    musicplayer.cpp \
    saveutils.cpp \
    searchablemultilistdatawiget.cpp \
    serverdatasender.cpp \
    underlinedrowdelegate.cpp \
    utils.cpp \
    xmlscript.cpp

HEADERS += \
    IgnoreHoverEventFilter.h \
    JsonRetrievers/JsonArrayRetriever.h \
    JsonRetrievers/JsonConditionRetriever.h \
    JsonRetrievers/JsonMapRetriever.h \
    JsonRetrievers/JsonNumberRetriever.h \
    JsonRetrievers/JsonRetriever.h \
    JsonRetrievers/JsonRetrieverHelper.h \
    JsonRetrievers/JsonStringRetriever.h \
    Section.h \
    charactersheet.h \
    dicehelper.h \
    drawablemap.h \
    fighttracker.h \
    interactivemap.h \
    jsonfilters/jsonfilter.h \
    jsontoqtxmbuilder.h \
    mainwindow.h \
    mapzonegraphicsobject.h \
    minimalscopeprofiler.h \
    monsterdelegate.h \
    musicplayer.h \
    saveutils.h \
    searchablemultilistdatawiget.h \
    serverdatasender.h \
    underlinedrowdelegate.h \
    utils.h \
    xmlscript.h

FORMS += \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES +=

QT += \
    quickwidgets \
    quick \
    network \
    positioning \
    core5compat \
    multimedia \
    xml \
    qml

RESOURCES += \
    qrc.qrc

RC_FILE = Morfal.rc
RC_ICONS = morfal.ico

TARGET = Morfal
