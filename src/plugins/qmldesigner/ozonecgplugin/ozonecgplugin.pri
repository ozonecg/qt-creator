QT *= qml quick core widgets
QT += quick-private


HEADERS += $$PWD/ozonecgplugin.h \
    $$PWD/assetsview/ozonecgassetsview.h \
    $$PWD/assetsview/ozonecgassetswidget.h \
    $$PWD/cmsdataagent.h
SOURCES += ozonecgplugin.cpp \
    $$PWD/assetsview/ozonecgassetsview.cpp \
    $$PWD/assetsview/ozonecgassetswidget.cpp \
    $$PWD/cmsdataagent.cpp

FORMS += \
    $$PWD/assetsview/ozonecgassetswidget.ui
