#-------------------------------------------------
#
# Project created by QtCreator 2015-07-23T13:52:43
#
#-------------------------------------------------

QT       += core gui

TARGET = augmentedReality
TEMPLATE = app

DEFINES += HAVE_CONFIG_H=1

INCLUDEPATH += /usr/include/vtk-5.8/

SOURCES += main.cpp\
        dcmCatalog.cpp\
        augreality.cpp \
        endoQuery.cpp \
    seriesdialog.cpp \
    vtkdicom.cpp

HEADERS  += augreality.h dcmCatalog.h \
    endoQuery.h \
    seriesdialog.h \
    vtkdicom.h

FORMS    += augreality.ui \
    seriesdialog.ui

unix:!macx:!symbian: LIBS += -lQVTK

unix:!macx:!symbian: LIBS += -lvtkCommon

unix:!macx:!symbian: LIBS += -lvtkIO

unix:!macx:!symbian: LIBS += -lvtkRendering

unix:!macx:!symbian: LIBS += -lvtkGraphics

unix:!macx:!symbian: LIBS += -lvtkFiltering

unix:!macx:!symbian: LIBS += -lvtkHybrid

unix:!macx:!symbian: LIBS += -lvtkVolumeRendering

unix:!macx:!symbian: LIBS += -ldcmdata

unix:!macx:!symbian: LIBS += -lofstd

unix:!macx:!symbian: LIBS += -loflog


