#-------------------------------------------------
#
# Project created by QtCreator 2015-10-20T14:09:13
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = raw_reader
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    rawreader.cpp \
    imageoutput.cpp

HEADERS  += mainwindow.h \
    rawreader.h \
    imageoutput.h

FORMS    += mainwindow.ui
