QT += core
QT -= gui

CONFIG += c++14

TARGET = ExternalMergeSort
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += main.cpp \
    externalmergesort.cpp

HEADERS += \
    externalmergesort.h \
    ThreadPool.h
