QT *= qml quick core widgets multimedia multimediawidgets
QT += quick-private


HEADERS += $$PWD/ozonecgplugin.h \
    $$PWD/assetspreview/o3assetspreview.h \
    $$PWD/assetspreview/o3assetspreviewwidget.h \
    $$PWD/assetsview/o3assetsview.h \
    $$PWD/assetsview/o3assetswidget.h \
    $$PWD/o3cmsdataagent.h
SOURCES += ozonecgplugin.cpp \
    $$PWD/assetspreview/o3assetspreview.cpp \
    $$PWD/assetspreview/o3assetspreviewwidget.cpp \
    $$PWD/assetsview/o3assetsview.cpp \
    $$PWD/assetsview/o3assetswidget.cpp \
    $$PWD/o3cmsdataagent.cpp

FORMS += \
    $$PWD/assetspreview/o3assetspreviewwidget.ui \
    $$PWD/assetsview/o3assetswidget.ui
