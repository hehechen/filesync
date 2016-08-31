QT += core
QT -= gui

CONFIG += c++11

TARGET = filesync
CONFIG += console
CONFIG -= app_bundle

LIBS +=  -L/usr/lib -lprotobuf -lcrypt -lpthread    \
         -L/home/chen/Desktop/work/build/release-install/lib -lmuduo_net -lmuduo_base   \
         -lz
INCLUDEPATH += /usr/local/include/google/protobuf   \
                /home/chen/Desktop/work/build/release-install/include

TEMPLATE = app

SOURCES += main.cpp \
    socket.cpp \
    sysutil.cpp \
    test/socketTest.cpp \
    test/filesendtest.cpp \
    test/prototest.cpp \
    test/muduo_test.cpp \
    syncserver.cpp \
    protobuf/filesync.pb.cc \
    codec.cpp \
    parseconfig.cpp \
    str_tool.cpp \
    test/parsetest.cpp \
    rsync.cpp \
    test/rsynctest.cpp \
    test/oobServer.cpp

HEADERS += \
    socket.h \
    common.h \
    sysutil.h \
    test/echo.h \
    syncserver.h \
    protobuf/filesync.pb.h \
    codec.h \
    parseconfig.h \
    str_tool.h \
    rsync.h

DISTFILES += \
    protobuf/filesync.proto
