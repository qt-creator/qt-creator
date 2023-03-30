add_qtc_plugin(SerialTerminal
  CONDITION TARGET Qt::SerialPort
  DEPENDS Qt::SerialPort
  PLUGIN_DEPENDS Core TextEditor
  SOURCES
    consolelineedit.cpp consolelineedit.h
    serialcontrol.cpp serialcontrol.h
    serialdevicemodel.cpp serialdevicemodel.h
    serialoutputpane.cpp serialoutputpane.h
    serialterminalconstants.h
    serialterminalplugin.cpp serialterminalplugin.h
    serialterminalsettings.cpp serialterminalsettings.h
    serialterminaltr.h
)
