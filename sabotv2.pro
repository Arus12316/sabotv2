#-------------------------------------------------
#
# Project created by QtCreator 2014-07-09T01:07:17
#
#-------------------------------------------------

QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = sabotv2
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    connection.cpp \
    parse.cpp \
    user.cpp \
    server.cpp \
    loginmultiusers.cpp \
    regex.c \
    createaccount.cpp \
    crypt.c \
    general.c

HEADERS  += mainwindow.h \
    connection.h \
    parse.h \
    user.h \
    server.h \
    loginmultiusers.h \
    regex.h \
    createaccount.h \
    crypt.h \
    general.h

FORMS    += mainwindow.ui \
    loginmultiusers.ui \
    createaccount.ui
CONFIG += c++11
