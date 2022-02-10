QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    findaxes.cpp \
    findcamera.cpp \
    main.cpp \
    pi.cpp \
    pi_controller.cpp \
    uEyeCamera.cpp

HEADERS += \
    findaxes.h \
    findcamera.h \
    pi.h \
    pi_controller.h \
    uEye.h \
    uEyeCamera.h

FORMS += \
    findaxes.ui \
    findcamera.ui \
    pi.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/./ -lPI_Mercury_GCS_DLL
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/./ -lPI_Mercury_GCS_DLLd
else:unix: LIBS += -L$$PWD/./ -lPI_Mercury_GCS_DLL

INCLUDEPATH += $$PWD/.
DEPENDPATH += $$PWD/.

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/./ -luEye_api_64
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/./ -luEye_api_64d

INCLUDEPATH += $$PWD/.
DEPENDPATH += $$PWD/.

win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/./libuEye_api_64.a
else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/./libuEye_api_64d.a
else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/./uEye_api_64.lib
else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/./uEye_api_64d.lib

  LIBS += -LC:\OpenCV\OpenCV_build\install\x64\vc16\lib -lopencv_core455 -lopencv_imgproc455 -lopencv_highgui455 -lopencv_imgcodecs455 -lopencv_videoio455 -lopencv_video455 -lopencv_calib3d455 -lopencv_photo455 -lopencv_features2d455
  INCLUDEPATH += C:\OpenCV\OpenCV_build\install\include
  DEPENDPATH += C:\OpenCV\OpenCV_build\install\include
