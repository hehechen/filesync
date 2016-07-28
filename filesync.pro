QT += core
QT -= gui

CONFIG += c++11

TARGET = filesync
CONFIG += console
CONFIG -= app_bundle

LIBS +=  -L/usr/lib -lprotobuf -lcrypt -lpthread
INCLUDEPATH += /usr/local/include/google/protobuf

TEMPLATE = app

SOURCES += main.cpp \
    socket.cpp \
    sysutil.cpp \
    test/socketTest.cpp \
    test/filesendtest.cpp \
    test/prototest.cpp \
    protobuf/filesync.init.pb.cc

HEADERS += \
    socket.h \
    common.h \
    sysutil.h \
    protobuf/sync.init.pb.h \
    protobuf/filesync.init.pb.h

DISTFILES += \
    protobuf/filesync.init.proto
