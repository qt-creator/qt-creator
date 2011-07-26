include(../ssh.pri)
QT += network

TARGET=shell
SOURCES=main.cpp shell.cpp ../remoteprocess/argumentscollector.cpp
HEADERS=shell.h ../remoteprocess/argumentscollector.h
