HEADERS += \
    $$PWD/ArcFaceEngine.h \
    $$PWD/arcfacemanager.h

SOURCES += \
    $$PWD/ArcFaceEngine.cpp \
    $$PWD/arcfacemanager.cpp

DISTFILES += \
    $$PWD/Readme.txt

RESOURCES += \
    $$PWD/rc.qrc

win32: LIBS += -L$$PWD/lib32/FreeSdk/ -llibarcsoft_face_engine

INCLUDEPATH += $$PWD/include/inc
DEPENDPATH += $$PWD/include/inc

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/lib32/opencv/lib/ -lopencv_core249
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/lib32/opencv/lib/ -lopencv_core249d

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/lib32/opencv/lib/ -llopencv_highgui249
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/lib32/opencv/lib/ -lopencv_highgui249d

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/lib32/opencv/lib/ -lopencv_imgproc249
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/lib32/opencv/lib/ -lopencv_imgproc249d

INCLUDEPATH += $$PWD/include/opencv/include
DEPENDPATH += $$PWD/include/opencv/include
