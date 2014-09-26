#-------------------------------------------------
#
# Project created by QtCreator 2014-07-09T01:07:17
#
#-------------------------------------------------

QT += core gui network testlib sql
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
raid.cpp \
    proxyscan.cpp \
    database.cpp

HEADERS += mainwindow.h \
connection.h \
user.h \
server.h \
loginmultiusers.h \
regex.h \
createaccount.h \
crypt.h \
general.h \
raid.h \
    proxyscan.h \
    database.h \
    schema.h

FORMS += mainwindow.ui \
loginmultiusers.ui \
createaccount.ui \
raid.ui \
proxyscan.ui

CONFIG += c++11
