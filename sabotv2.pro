#-------------------------------------------------
#
# Project created by QtCreator 2014-07-09T01:07:17
#
#-------------------------------------------------

QT += core gui network testlib sql
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = sabotv2
TEMPLATE = app

SOURCES += $$PWD/main.cpp\
$$PWD/mainwindow.cpp \
$$PWD/connection.cpp \
$$PWD/user.cpp \
$$PWD/server.cpp \
$$PWD/loginmultiusers.cpp \
$$PWD/regex.c \
$$PWD/createaccount.cpp \
$$PWD/crypt.c \
$$PWD/general.c \
$$PWD/raid.cpp \
    $$PWD/proxyscan.cpp \
    $$PWD/database.cpp \
    $$PWD/calc.c

HEADERS += $$PWD/mainwindow.h \
$$PWD/connection.h \
$$PWD/user.h \
$$PWD/server.h \
$$PWD/loginmultiusers.h \
$$PWD/regex.h \
$$PWD/createaccount.h \
$$PWD/crypt.h \
$$PWD/general.h \
$$PWD/raid.h \
    $$PWD/proxyscan.h \
    $$PWD/database.h \
    $$PWD/schema.h \
    $$PWD/calc.h

FORMS += $$PWD/mainwindow.ui \
$$PWD/loginmultiusers.ui \
$$PWD/createaccount.ui \
$$PWD/raid.ui \
$$PWD/proxyscan.ui

CONFIG += c++11

QMAKE_CFLAGS -= -O
QMAKE_CFLAGS -= -O1
QMAKE_CFLAGS -= -O2
QMAKE_CFLAGS *= -O3
