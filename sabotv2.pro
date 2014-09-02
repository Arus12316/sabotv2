#-------------------------------------------------
#
# Project created by QtCreator 2014-07-09T01:07:17
#
#-------------------------------------------------

QT       += core gui network testlib

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = sabotv2
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    connection.cpp \
    user.cpp \
    server.cpp \
    loginmultiusers.cpp \
    regex.c \
    createaccount.cpp \
    crypt.c \
    general.c \
    database.cpp \
    raid.cpp

HEADERS  += mainwindow.h \
    connection.h \
    user.h \
    server.h \
    loginmultiusers.h \
    regex.h \
    createaccount.h \
    crypt.h \
    general.h \
    database.h \
    raid.h

FORMS    += mainwindow.ui \
    loginmultiusers.ui \
    createaccount.ui \
    raid.ui
CONFIG += c++11
QMAKE_CC= gcc -std=c11

