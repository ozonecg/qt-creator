TARGET = ozonecgplugin
TEMPLATE = lib
CONFIG += plugin

include (../../../../qtcreator.pri)
include (../plugindestdir.pri)
include (../designercore/iwidgetplugin.pri)
include (../qmldesigner_dependencies.pri)
include (ozonecgplugin.pri)

LIBS += -L$$IDE_PLUGIN_PATH
LIBS += -l$$qtLibraryName(QmlDesigner)
LIBS += -l$$qtLibraryName(ExtensionSystem)
LIBS += -l$$qtLibraryName(Core)
LIBS += -l$$qtLibraryName(ProjectExplorer)
LIBS += -l$$qtLibraryName(Utils)

RESOURCES += \
    ozonecgplugin.qrc

