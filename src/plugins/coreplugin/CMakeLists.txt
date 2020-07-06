option(SHOW_BUILD_DATE "Show build date in about dialog" OFF)

add_qtc_plugin(Core
  DEPENDS Qt5::PrintSupport Qt5::Qml Qt5::Sql Qt5::Gui Qt5::GuiPrivate
  PUBLIC_DEPENDS Aggregation ExtensionSystem Utils app_version
  SOURCES
    actionmanager/actioncontainer.cpp actionmanager/actioncontainer.h actionmanager/actioncontainer_p.h
    actionmanager/actionmanager.cpp actionmanager/actionmanager.h actionmanager/actionmanager_p.h
    actionmanager/command.cpp actionmanager/command.h actionmanager/command_p.h
    actionmanager/commandbutton.cpp actionmanager/commandbutton.h
    actionmanager/commandmappings.cpp actionmanager/commandmappings.h
    actionmanager/commandsfile.cpp actionmanager/commandsfile.h
    basefilewizard.cpp basefilewizard.h
    basefilewizardfactory.cpp basefilewizardfactory.h
    core.qrc
    core_global.h
    coreconstants.h
    coreicons.cpp coreicons.h
    corejsextensions.cpp corejsextensions.h
    coreplugin.cpp coreplugin.h
    designmode.cpp designmode.h
    dialogs/addtovcsdialog.cpp dialogs/addtovcsdialog.h dialogs/addtovcsdialog.ui
    dialogs/codecselector.cpp dialogs/codecselector.h
    dialogs/externaltoolconfig.cpp dialogs/externaltoolconfig.h dialogs/externaltoolconfig.ui
    dialogs/filepropertiesdialog.cpp dialogs/filepropertiesdialog.h dialogs/filepropertiesdialog.ui
    dialogs/ioptionspage.cpp dialogs/ioptionspage.h
    dialogs/newdialog.cpp dialogs/newdialog.h dialogs/newdialog.ui
    dialogs/openwithdialog.cpp dialogs/openwithdialog.h dialogs/openwithdialog.ui
    dialogs/promptoverwritedialog.cpp dialogs/promptoverwritedialog.h
    dialogs/readonlyfilesdialog.cpp dialogs/readonlyfilesdialog.h dialogs/readonlyfilesdialog.ui
    dialogs/restartdialog.cpp dialogs/restartdialog.h
    dialogs/saveitemsdialog.cpp dialogs/saveitemsdialog.h dialogs/saveitemsdialog.ui
    dialogs/settingsdialog.cpp dialogs/settingsdialog.h
    dialogs/shortcutsettings.cpp dialogs/shortcutsettings.h
    diffservice.cpp diffservice.h
    documentmanager.cpp documentmanager.h
    editmode.cpp editmode.h
    editormanager/documentmodel.cpp editormanager/documentmodel.h editormanager/documentmodel_p.h
    editormanager/editorarea.cpp editormanager/editorarea.h
    editormanager/editormanager.cpp editormanager/editormanager.h editormanager/editormanager_p.h
    editormanager/editorview.cpp editormanager/editorview.h
    editormanager/editorwindow.cpp editormanager/editorwindow.h
    editormanager/ieditor.cpp editormanager/ieditor.h
    editormanager/ieditorfactory.cpp editormanager/ieditorfactory.h editormanager/ieditorfactory_p.h
    editormanager/iexternaleditor.cpp editormanager/iexternaleditor.h
    editormanager/openeditorsview.cpp editormanager/openeditorsview.h
    editormanager/openeditorswindow.cpp editormanager/openeditorswindow.h
    editormanager/systemeditor.cpp editormanager/systemeditor.h
    editortoolbar.cpp editortoolbar.h
    externaltool.cpp externaltool.h
    externaltoolmanager.cpp externaltoolmanager.h
    fancyactionbar.cpp fancyactionbar.h
    fancyactionbar.qrc
    fancytabwidget.cpp fancytabwidget.h
    featureprovider.cpp featureprovider.h
    fileiconprovider.cpp fileiconprovider.h
    fileutils.cpp fileutils.h
    find/basetextfind.cpp find/basetextfind.h
    find/currentdocumentfind.cpp find/currentdocumentfind.h
    find/find.qrc
    find/finddialog.ui
    find/findplugin.cpp find/findplugin.h
    find/findtoolbar.cpp find/findtoolbar.h
    find/findtoolwindow.cpp find/findtoolwindow.h
    find/findwidget.ui
    find/highlightscrollbarcontroller.cpp find/highlightscrollbarcontroller.h
    find/ifindfilter.cpp find/ifindfilter.h
    find/ifindsupport.cpp find/ifindsupport.h
    find/itemviewfind.cpp find/itemviewfind.h
    find/optionspopup.cpp find/optionspopup.h
    find/searchresultcolor.h
    find/searchresultitem.h
    find/searchresulttreeitemdelegate.cpp find/searchresulttreeitemdelegate.h
    find/searchresulttreeitemroles.h
    find/searchresulttreeitems.cpp find/searchresulttreeitems.h
    find/searchresulttreemodel.cpp find/searchresulttreemodel.h
    find/searchresulttreeview.cpp find/searchresulttreeview.h
    find/searchresultwidget.cpp find/searchresultwidget.h
    find/searchresultwindow.cpp find/searchresultwindow.h
    find/textfindconstants.h
    findplaceholder.cpp findplaceholder.h
    generalsettings.cpp generalsettings.h generalsettings.ui
    generatedfile.cpp generatedfile.h
    helpitem.cpp helpitem.h
    helpmanager.cpp helpmanager.h helpmanager_implementation.h
    icontext.cpp icontext.h
    icore.cpp icore.h
    id.cpp id.h
    idocument.cpp idocument.h
    idocumentfactory.cpp idocumentfactory.h
    ifilewizardextension.h
    imode.cpp imode.h
    inavigationwidgetfactory.cpp inavigationwidgetfactory.h
    ioutputpane.cpp ioutputpane.h
    iversioncontrol.cpp iversioncontrol.h
    iwelcomepage.cpp iwelcomepage.h
    iwizardfactory.cpp iwizardfactory.h
    jsexpander.cpp jsexpander.h
    locator/basefilefilter.cpp locator/basefilefilter.h
    locator/commandlocator.cpp locator/commandlocator.h
    locator/directoryfilter.cpp locator/directoryfilter.h locator/directoryfilter.ui
    locator/executefilter.cpp locator/executefilter.h
    locator/externaltoolsfilter.cpp locator/externaltoolsfilter.h
    locator/filesystemfilter.cpp locator/filesystemfilter.h locator/filesystemfilter.ui
    locator/ilocatorfilter.cpp locator/ilocatorfilter.h
    locator/locator.cpp locator/locator.h
    locator/locatorconstants.h
    locator/locatorfiltersfilter.cpp locator/locatorfiltersfilter.h
    locator/locatormanager.cpp locator/locatormanager.h
    locator/locatorsearchutils.cpp locator/locatorsearchutils.h
    locator/locatorsettingspage.cpp locator/locatorsettingspage.h locator/locatorsettingspage.ui
    locator/locatorwidget.cpp locator/locatorwidget.h
    locator/opendocumentsfilter.cpp locator/opendocumentsfilter.h
    locator/urllocatorfilter.cpp locator/urllocatorfilter.h locator/urllocatorfilter.ui
    mainwindow.cpp mainwindow.h
    manhattanstyle.cpp manhattanstyle.h
    menubarfilter.cpp menubarfilter.h
    messagebox.cpp messagebox.h
    messagemanager.cpp messagemanager.h
    messageoutputwindow.cpp messageoutputwindow.h
    mimetypemagicdialog.cpp mimetypemagicdialog.h mimetypemagicdialog.ui
    mimetypesettings.cpp mimetypesettings.h
    mimetypesettingspage.ui
    minisplitter.cpp minisplitter.h
    modemanager.cpp modemanager.h
    navigationsubwidget.cpp navigationsubwidget.h
    navigationwidget.cpp navigationwidget.h
    opendocumentstreeview.cpp opendocumentstreeview.h
    outputpane.cpp outputpane.h
    outputpanemanager.cpp outputpanemanager.h
    outputwindow.cpp outputwindow.h
    patchtool.cpp patchtool.h
    plugindialog.cpp plugindialog.h
    plugininstallwizard.cpp plugininstallwizard.h
    progressmanager/futureprogress.cpp progressmanager/futureprogress.h
    progressmanager/progressbar.cpp progressmanager/progressbar.h
    progressmanager/progressmanager.cpp progressmanager/progressmanager.h progressmanager/progressmanager_p.h
    progressmanager/progressview.cpp progressmanager/progressview.h
    reaper.cpp reaper.h reaper_p.h
    rightpane.cpp rightpane.h
    settingsdatabase.cpp settingsdatabase.h
    shellcommand.cpp shellcommand.h
    sidebar.cpp sidebar.h
    sidebarwidget.cpp sidebarwidget.h
    statusbarmanager.cpp statusbarmanager.h
    styleanimator.cpp styleanimator.h
    systemsettings.cpp systemsettings.h systemsettings.ui
    textdocument.cpp textdocument.h
    themechooser.cpp themechooser.h
    variablechooser.cpp variablechooser.h
    vcsmanager.cpp vcsmanager.h
    versiondialog.cpp versiondialog.h
    welcomepagehelper.cpp welcomepagehelper.h
    windowsupport.cpp windowsupport.h
  EXPLICIT_MOC dialogs/filepropertiesdialog.h
)

extend_qtc_plugin(Core
  CONDITION SHOW_BUILD_DATE
  DEFINES QTC_SHOW_BUILD_DATE
)

extend_qtc_plugin(Core
  CONDITION WITH_TESTS
  SOURCES
    locator/locator_test.cpp
    locator/locatorfiltertest.cpp locator/locatorfiltertest.h
    testdatadir.cpp testdatadir.h
)

extend_qtc_plugin(Core
  CONDITION WIN32
  SOURCES progressmanager/progressmanager_win.cpp
)

extend_qtc_plugin(Core
  CONDITION APPLE
  DEPENDS ${FWAppKit}
  SOURCES
    progressmanager/progressmanager_mac.mm
    locator/spotlightlocatorfilter.h locator/spotlightlocatorfilter.mm
)

extend_qtc_plugin(Core
  CONDITION (NOT WIN32) AND (NOT APPLE)
  SOURCES progressmanager/progressmanager_x11.cpp
)

extend_qtc_plugin(Core
  CONDITION Qt5_VERSION VERSION_GREATER_EQUAL 5.14.0
  FEATURE_INFO "Script Locator filter"
  DEFINES WITH_JAVASCRIPTFILTER
  SOURCES
    locator/javascriptfilter.cpp locator/javascriptfilter.h
)
