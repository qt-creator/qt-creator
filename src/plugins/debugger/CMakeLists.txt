add_qtc_plugin(Debugger
  DEPENDS LanguageUtils QmlDebug QmlJS QtcSsh registryaccess
  PLUGIN_DEPENDS Core CppTools ProjectExplorer QtSupport TextEditor
  PLUGIN_RECOMMENDS QmakeProjectManager
  SOURCES
    analyzer/analyzerbase.qrc
    analyzer/analyzerconstants.h
    analyzer/analyzericons.h
    analyzer/analyzermanager.h
    analyzer/analyzerrunconfigwidget.cpp analyzer/analyzerrunconfigwidget.h
    analyzer/analyzerutils.cpp analyzer/analyzerutils.h
    analyzer/detailederrorview.cpp analyzer/detailederrorview.h
    analyzer/diagnosticlocation.cpp analyzer/diagnosticlocation.h
    analyzer/startremotedialog.cpp analyzer/startremotedialog.h
    breakhandler.cpp breakhandler.h
    breakpoint.cpp breakpoint.h
    cdb/cdbengine.cpp cdb/cdbengine.h
    cdb/cdboptionspage.cpp cdb/cdboptionspage.h
    cdb/cdboptionspagewidget.ui
    cdb/cdbparsehelpers.cpp cdb/cdbparsehelpers.h
    cdb/stringinputstream.cpp cdb/stringinputstream.h
    commonoptionspage.cpp commonoptionspage.h
    console/console.cpp console/console.h
    console/consoleedit.cpp console/consoleedit.h
    console/consoleitem.cpp console/consoleitem.h
    console/consoleitemdelegate.cpp console/consoleitemdelegate.h
    console/consoleitemmodel.cpp console/consoleitemmodel.h
    console/consoleproxymodel.cpp console/consoleproxymodel.h
    console/consoleview.cpp console/consoleview.h
    debugger.qrc
    debugger_global.h
    debuggeractions.cpp debuggeractions.h
    debuggerconstants.h
    debuggercore.h
    debuggerdialogs.cpp debuggerdialogs.h
    debuggerengine.cpp debuggerengine.h
    debuggericons.cpp debuggericons.h
    debuggerinternalconstants.h
    debuggeritem.cpp debuggeritem.h
    debuggeritemmanager.cpp debuggeritemmanager.h
    debuggerkitinformation.cpp debuggerkitinformation.h
    debuggermainwindow.cpp debuggermainwindow.h
    debuggerplugin.cpp debuggerplugin.h
    debuggerprotocol.cpp debuggerprotocol.h
    debuggerrunconfigurationaspect.cpp debuggerrunconfigurationaspect.h
    debuggerruncontrol.cpp debuggerruncontrol.h
    debuggersourcepathmappingwidget.cpp debuggersourcepathmappingwidget.h
    debuggertooltipmanager.cpp debuggertooltipmanager.h
    disassembleragent.cpp disassembleragent.h
    disassemblerlines.cpp disassemblerlines.h
    gdb/gdbengine.cpp gdb/gdbengine.h
    gdb/gdboptionspage.cpp
    imageviewer.cpp imageviewer.h
    enginemanager.cpp enginemanager.h
    lldb/lldbengine.cpp lldb/lldbengine.h
    loadcoredialog.cpp loadcoredialog.h
    localsandexpressionswindow.cpp localsandexpressionswindow.h
    logwindow.cpp logwindow.h
    memoryagent.cpp memoryagent.h
    moduleshandler.cpp moduleshandler.h
    namedemangler/demanglerexceptions.h
    namedemangler/globalparsestate.cpp namedemangler/globalparsestate.h
    namedemangler/namedemangler.cpp namedemangler/namedemangler.h
    namedemangler/parsetreenodes.cpp namedemangler/parsetreenodes.h
    outputcollector.cpp outputcollector.h
    pdb/pdbengine.cpp pdb/pdbengine.h
    peripheralregisterhandler.cpp peripheralregisterhandler.h
    procinterrupt.cpp procinterrupt.h
    qml/interactiveinterpreter.cpp qml/interactiveinterpreter.h
    #qml/qmlcppengine.cpp qml/qmlcppengine.h
    qml/qmlengine.cpp qml/qmlengine.h
    qml/qmlengineutils.cpp qml/qmlengineutils.h
    qml/qmlinspectoragent.cpp qml/qmlinspectoragent.h
    qml/qmlv8debuggerclientconstants.h
    registerhandler.cpp registerhandler.h
    shared/backtrace.cpp shared/backtrace.h
    shared/cdbsymbolpathlisteditor.cpp shared/cdbsymbolpathlisteditor.h
    shared/hostutils.cpp shared/hostutils.h
    shared/peutils.cpp shared/peutils.h
    shared/symbolpathsdialog.cpp shared/symbolpathsdialog.h shared/symbolpathsdialog.ui
    simplifytype.cpp simplifytype.h
    sourceagent.cpp sourceagent.h
    sourcefileshandler.cpp sourcefileshandler.h
    sourceutils.cpp sourceutils.h
    stackframe.cpp stackframe.h
    stackhandler.cpp stackhandler.h
    stackwindow.cpp stackwindow.h
    terminal.cpp terminal.h
    threaddata.h
    threadshandler.cpp threadshandler.h
    unstartedappwatcherdialog.cpp unstartedappwatcherdialog.h
    uvsc/uvscclient.cpp uvsc/uvscclient.h
    uvsc/uvscdatatypes.h
    uvsc/uvscengine.cpp uvsc/uvscengine.h
    uvsc/uvscfunctions.h
    uvsc/uvscutils.cpp uvsc/uvscutils.h
    watchdata.cpp watchdata.h
    watchdelegatewidgets.cpp watchdelegatewidgets.h
    watchhandler.cpp watchhandler.h
    watchutils.cpp watchutils.h
    watchwindow.cpp watchwindow.h
)

extend_qtc_plugin(Debugger
  CONDITION WIN32
  SOURCES registerpostmortemaction.cpp registerpostmortemaction.h
  DEFINES UNICODE _UNICODE
)

extend_qtc_plugin(Debugger
  CONDITION WITH_TESTS
  SOURCES
    debuggerunittests.qrc
    unit-tests/simple/main.cpp
)
