QT += core
QT -= gui

CONFIG += c++11

TARGET = filesync
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += main.cpp \
    socket.cpp \
    sysutil.cpp \
    test/socketTest.cpp \
    test/filesendtest.cpp

HEADERS += \
    socket.h \
    common.h \
    sysutil.h
