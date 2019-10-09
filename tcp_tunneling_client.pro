TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt
LIBS += -lnetfilter_queue
SOURCES += \
        cal_checksum.cpp \
        get_my_info.cpp \
        main.cpp

HEADERS += \
    cal_checksum.h \
    get_my_info.h \
    include.h \
    protocol_structure.h
