TEMPLATE = lib
CONFIG += qt plugin
QT += declarative
QT += script
 
TARGET  = styleplugin
include(../../../../../qtcreator.pri)
DESTDIR = $$IDE_DATA_PATH/welcomescreen/components/plugin
OBJECTS_DIR = tmp
MOC_DIR = tmp

HEADERS += qtmenu.h \
           qtmenubar.h \
           qtmenuitem.h \
           qrangemodel_p.h \
           qrangemodel.h \
           qstyleplugin.h \
           qdeclarativefolderlistmodel.h \
           qstyleitem.h \
           qwheelarea.h

SOURCES += qtmenu.cpp \
           qtmenubar.cpp \
           qtmenuitem.cpp \
           qrangemodel.cpp \
	   qstyleplugin.cpp \
           qdeclarativefolderlistmodel.cpp \
           qstyleitem.cpp \
           qwheelarea.cpp
           

OTHER_FILES += \
    ../gallery.qml \
    ../widgets/Tab.qml \
    ../widgets/TabBar.qml \
    ../widgets/TabFrame.qml \
    ../Button.qml \
    ../ButtonRow.qml \
    ../CheckBox.qml \
    ../ChoiceList.qml \
    ../components.pro \
    ../ContextMenu.qml \
    ../Dial.qml \
    ../Frame.qml \
    ../GroupBox.qml \
    ../Menu.qml \
    ../ProgressBar.qml \
    ../RadioButton.qml \
    ../ScrollArea.qml \
    ../ScrollBar.qml \
    ../Slider.qml \
    ../SpinBox.qml \
    ../Switch.qml \
    ../Tab.qml \
    ../TableView.qml \
    ../TabBar.qml \
    ../TabFrame.qml \
    ../TextArea.qml \
    ../TextField.qml \
    ../TextScrollArea.qml \
    ../ToolBar.qml \
    ../ToolButton.qml \
    ../custom/BasicButton.qml \
    ../custom/BusyIndicator.qml \
    ../custom/Button.qml \
    ../custom/ButtonColumn.qml \
    ../custom/ButtonGroup.js \
    ../custom/ButtonRow.qml \
    ../custom/CheckBox.qml \
    ../custom/ChoiceList.qml \
    ../custom/ProgressBar.qml \
    ../custom/Slider.qml \
    ../custom/SpinBox.qml \
    ../custom/TextField.qml \
    ../../examples/Browser.qml \
    ../../examples/Panel.qml \
    ../../examples/ModelView.qml \
    ../../examples/Gallery.qml
