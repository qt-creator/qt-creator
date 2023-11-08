add_qtc_library(Utils
  DEPENDS Tasking Qt::Qml Qt::Xml Spinner
  PUBLIC_DEPENDS
    Qt::Concurrent Qt::Core Qt::Network Qt::Gui Qt::Widgets
    Qt::Core5Compat
    $<$<BOOL:${QTC_STATIC_BUILD}>:Tasking>
    $<$<BOOL:${QTC_STATIC_BUILD}>:Spinner>
  SOURCES
    ../3rdparty/span/span.hpp
    ../3rdparty/tl_expected/include/tl/expected.hpp
    algorithm.h
    ansiescapecodehandler.cpp ansiescapecodehandler.h
    appinfo.cpp appinfo.h
    appmainwindow.cpp appmainwindow.h
    array.h
    aspects.cpp aspects.h
    async.cpp async.h
    basetreeview.cpp basetreeview.h
    benchmarker.cpp benchmarker.h
    buildablehelperlibrary.cpp buildablehelperlibrary.h
    camelcasecursor.cpp camelcasecursor.h
    categorysortfiltermodel.cpp categorysortfiltermodel.h
    changeset.cpp changeset.h
    checkablemessagebox.cpp checkablemessagebox.h
    classnamevalidatinglineedit.cpp classnamevalidatinglineedit.h
    clangutils.cpp clangutils.h
    codegeneration.cpp codegeneration.h
    commandline.cpp commandline.h
    completinglineedit.cpp completinglineedit.h
    completingtextedit.cpp completingtextedit.h
    cpplanguage_details.h
    crumblepath.cpp crumblepath.h
    delegates.cpp delegates.h
    detailsbutton.cpp detailsbutton.h
    detailswidget.cpp detailswidget.h
    devicefileaccess.cpp devicefileaccess.h
    deviceshell.cpp deviceshell.h
    differ.cpp differ.h
    displayname.cpp displayname.h
    dropsupport.cpp dropsupport.h
    elfreader.cpp elfreader.h
    elidinglabel.cpp elidinglabel.h
    environment.cpp environment.h
    environmentdialog.cpp environmentdialog.h
    environmentfwd.h
    environmentmodel.cpp environmentmodel.h
    execmenu.cpp execmenu.h
    expected.h
    externalterminalprocessimpl.cpp externalterminalprocessimpl.h
    fadingindicator.cpp fadingindicator.h
    faketooltip.cpp faketooltip.h
    fancylineedit.cpp fancylineedit.h
    fancymainwindow.cpp fancymainwindow.h
    filecrumblabel.cpp filecrumblabel.h
    fileinprojectfinder.cpp fileinprojectfinder.h
    filenamevalidatinglineedit.cpp filenamevalidatinglineedit.h
    filepath.cpp filepath.h
    filepathinfo.h
    filesearch.cpp filesearch.h
    filestreamer.cpp filestreamer.h
    filestreamermanager.cpp filestreamermanager.h
    filesystemmodel.cpp filesystemmodel.h
    filesystemwatcher.cpp filesystemwatcher.h
    fileutils.cpp fileutils.h
    filewizardpage.cpp filewizardpage.h
    flowlayout.cpp flowlayout.h
    fsengine/fsengine.cpp fsengine/fsengine.h
    fsengine/fileiconprovider.cpp fsengine/fileiconprovider.h
    futuresynchronizer.cpp futuresynchronizer.h
    fuzzymatcher.cpp fuzzymatcher.h
    genericconstants.h
    globalfilechangeblocker.cpp globalfilechangeblocker.h
    guard.cpp guard.h
    headerviewstretcher.cpp headerviewstretcher.h
    highlightingitemdelegate.cpp highlightingitemdelegate.h
    historycompleter.cpp historycompleter.h
    hostosinfo.cpp hostosinfo.h
    htmldocextractor.cpp htmldocextractor.h
    icon.cpp icon.h
    iconbutton.cpp iconbutton.h
    id.cpp id.h
    indexedcontainerproxyconstiterator.h
    infobar.cpp infobar.h
    infolabel.cpp infolabel.h
    itemviews.cpp itemviews.h
    jsontreeitem.cpp jsontreeitem.h
    launcherinterface.cpp launcherinterface.h
    launcherpackets.cpp launcherpackets.h
    launchersocket.cpp launchersocket.h
    layoutbuilder.cpp layoutbuilder.h
    link.cpp link.h
    listmodel.h
    listutils.h
    macroexpander.cpp macroexpander.h
    mathutils.cpp mathutils.h
    mimeutils.h
    minimizableinfobars.cpp
    minimizableinfobars.h
    multitextcursor.cpp multitextcursor.h
    namevaluedictionary.cpp namevaluedictionary.h
    namevaluedictionary.cpp namevaluedictionary.h
    namevalueitem.cpp namevalueitem.h
    namevalueitem.cpp namevalueitem.h
    namevaluemodel.cpp namevaluemodel.h
    namevaluemodel.cpp namevaluemodel.h
    namevaluesdialog.cpp namevaluesdialog.h
    namevaluesdialog.cpp namevaluesdialog.h
    namevaluevalidator.cpp namevaluevalidator.h
    namevaluevalidator.cpp namevaluevalidator.h
    navigationtreeview.cpp navigationtreeview.h
    networkaccessmanager.cpp networkaccessmanager.h
    optionpushbutton.h
    optionpushbutton.cpp
    osspecificaspects.h
    outputformat.h
    outputformatter.cpp outputformatter.h
    overlaywidget.cpp overlaywidget.h
    overridecursor.cpp overridecursor.h
    parameteraction.cpp parameteraction.h
    passworddialog.cpp passworddialog.h
    pathchooser.cpp pathchooser.h
    pathlisteditor.cpp pathlisteditor.h
    persistentcachestore.cpp persistentcachestore.h
    persistentsettings.cpp persistentsettings.h
    pointeralgorithm.h
    port.cpp port.h
    portlist.cpp portlist.h
    predicates.h
    process.cpp process.h
    processenums.h
    processhandle.cpp processhandle.h
    processinfo.cpp processinfo.h
    processinterface.cpp processinterface.h
    processreaper.cpp processreaper.h
    processutils.cpp processutils.h
    progressindicator.cpp progressindicator.h
    projectintropage.cpp projectintropage.h
    proxyaction.cpp proxyaction.h
    qrcparser.cpp qrcparser.h
    qtcassert.cpp qtcassert.h
    qtcolorbutton.cpp qtcolorbutton.h
    qtcsettings.cpp qtcsettings.h
    ranges.h
    reloadpromptutils.cpp reloadpromptutils.h
    removefiledialog.cpp removefiledialog.h
    savefile.cpp savefile.h
    scopedswap.h
    scopedtimer.cpp scopedtimer.h
    searchresultitem.cpp searchresultitem.h
    set_algorithm.h
    settingsaccessor.cpp settingsaccessor.h
    settingsselector.cpp settingsselector.h
    singleton.cpp singleton.h
    sizedarray.h
    smallstring.h
    smallstringfwd.h
    smallstringio.h
    smallstringiterator.h
    smallstringlayout.h
    smallstringliteral.h
    smallstringmemory.h
    smallstringvector.h
    smallstringview.h
    sortfiltermodel.h
    span.h
    statuslabel.cpp statuslabel.h
    store.cpp store.h
    storekey.h
    stringtable.cpp stringtable.h
    stringutils.cpp stringutils.h
    styleanimator.cpp styleanimator.h
    styledbar.cpp styledbar.h
    stylehelper.cpp stylehelper.h
    templateengine.cpp templateengine.h
    temporarydirectory.cpp temporarydirectory.h
    temporaryfile.cpp temporaryfile.h
    terminalcommand.cpp terminalcommand.h
    terminalhooks.cpp terminalhooks.h
    terminalinterface.cpp terminalinterface.h
    textfieldcheckbox.cpp textfieldcheckbox.h
    textfieldcombobox.cpp textfieldcombobox.h
    textfileformat.cpp textfileformat.h
    textutils.cpp textutils.h
    theme/theme.cpp theme/theme.h theme/theme_p.h
    threadutils.cpp threadutils.h
    tooltip/effects.h
    tooltip/tips.cpp tooltip/tips.h
    tooltip/tooltip.cpp tooltip/tooltip.h
    touchbar/touchbar.h
    transientscroll.cpp transientscroll.h
    treemodel.cpp treemodel.h
    treeviewcombobox.cpp treeviewcombobox.h
    unarchiver.cpp unarchiver.h
    uncommentselection.cpp uncommentselection.h
    uniqueobjectptr.h
    unixutils.cpp unixutils.h
    url.cpp url.h
    utils.qrc
    utils_global.h
    utilstr.h
    utilsicons.cpp utilsicons.h
    utiltypes.h
    variablechooser.cpp variablechooser.h
    winutils.cpp winutils.h
    wizard.cpp wizard.h
    wizardpage.cpp wizardpage.h
)

extend_qtc_library(Utils
  SOURCES_PREFIX mimetypes2
  PUBLIC_INCLUDES mimetypes2
  SOURCES
    mimedatabase.cpp
    mimedatabase.h
    mimedatabase_p.h
    mimeglobpattern.cpp
    mimeglobpattern_p.h
    mimemagicrule.cpp
    mimemagicrule_p.h
    mimemagicrulematcher.cpp
    mimemagicrulematcher_p.h
    mimeprovider.cpp
    mimeprovider_p.h
    mimetype.cpp
    mimetype.h
    mimetype_p.h
    mimetypeparser.cpp
    mimetypeparser_p.h
    mimeutils.cpp
)

if (MSVC)
    find_library(DbgHelpLib dbghelp)
endif()

extend_qtc_library(Utils CONDITION MSVC
  DEPENDS ${DbgHelpLib}
)

extend_qtc_library(Utils CONDITION WIN32
  SOURCES
    touchbar/touchbar.cpp
  DEPENDS
    user32 iphlpapi ws2_32 shell32 ole32
  DEFINES
    _UNICODE UNICODE
  PUBLIC_DEFINES
    _CRT_SECURE_NO_WARNINGS _SCL_SECURE_NO_WARNINGS
)

extend_qtc_library(Utils CONDITION APPLE
  SOURCES
    fileutils_mac.mm fileutils_mac.h
    processhandle_mac.mm
    theme/theme_mac.mm theme/theme_mac.h
    touchbar/touchbar_appdelegate_mac.mm touchbar/touchbar_appdelegate_mac_p.h
    touchbar/touchbar_mac.mm touchbar/touchbar_mac_p.h
  DEPENDS
    ${FWFoundation} ${FWAppKit}
)

extend_qtc_library(Utils CONDITION UNIX AND NOT APPLE
  SOURCES
    touchbar/touchbar.cpp
)

extend_qtc_library(Utils
  CONDITION TARGET Qt::CorePrivate
  DEPENDS Qt::CorePrivate ptyqt
  DEFINES QTC_UTILS_WITH_FSENGINE
  SOURCES fsengine/fsengine_impl.cpp
          fsengine/fsengine_impl.h
          fsengine/diriterator.h
          fsengine/fileiteratordevicesappender.h
          fsengine/rootinjectfsengine.h
          fsengine/fixedlistfsengine.h
          fsengine/fsenginehandler.cpp
          fsengine/fsenginehandler.h
          fsengine/filepathinfocache.h
)

extend_qtc_library(Utils
  CONDITION TARGET Nanotrace
  DEPENDS Nanotrace
)

if (WIN32)
  add_qtc_executable(qtcreator_ctrlc_stub
    DEPENDS user32 shell32
    DEFINES _UNICODE UNICODE _CRT_SECURE_NO_WARNINGS
    SOURCES
      process_ctrlc_stub.cpp
  )
endif()
