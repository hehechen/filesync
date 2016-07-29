QT += core
QT -= gui

CONFIG += c++11

TARGET = filesync
CONFIG += console
CONFIG -= app_bundle

LIBS +=  -L/usr/lib -lprotobuf -lcrypt -lpthread    \
         -L/home/chen/Desktop/work/build/release-install/lib -lmuduo_net -lmuduo_base
INCLUDEPATH += /usr/local/include/google/protobuf   \
                /home/chen/Desktop/work/build/release-install/include

TEMPLATE = app

SOURCES += main.cpp \
    socket.cpp \
    sysutil.cpp \
    test/socketTest.cpp \
    test/filesendtest.cpp \
    test/prototest.cpp \
    protobuf/filesync.init.pb.cc \
    test/muduo_test.cpp \
    syncserver.cpp

HEADERS += \
    socket.h \
    common.h \
    sysutil.h \
    protobuf/filesync.init.pb.h \
    test/echo.h \
    syncserver.h

DISTFILES += \
    protobuf/filesync.init.proto
