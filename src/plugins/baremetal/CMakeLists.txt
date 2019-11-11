add_qtc_plugin(BareMetal
  DEPENDS QtcSsh
  PLUGIN_DEPENDS Core Debugger ProjectExplorer
  SOURCES
    baremetal.qrc
    baremetalconstants.h
    baremetalcustomrunconfiguration.cpp baremetalcustomrunconfiguration.h
    baremetaldebugsupport.cpp baremetaldebugsupport.h
    baremetaldevice.cpp baremetaldevice.h
    baremetaldeviceconfigurationwidget.cpp baremetaldeviceconfigurationwidget.h
    baremetaldeviceconfigurationwizard.cpp baremetaldeviceconfigurationwizard.h
    baremetaldeviceconfigurationwizardpages.cpp baremetaldeviceconfigurationwizardpages.h
    baremetalplugin.cpp baremetalplugin.h
    baremetalrunconfiguration.cpp baremetalrunconfiguration.h
    debugserverproviderchooser.cpp debugserverproviderchooser.h
    debugserverprovidermanager.cpp debugserverprovidermanager.h
    debugserverproviderssettingspage.cpp debugserverproviderssettingspage.h
    debugservers/gdb/defaultgdbserverprovider.cpp debugservers/gdb/defaultgdbserverprovider.h
    debugservers/gdb/gdbserverprovider.cpp debugservers/gdb/gdbserverprovider.h
    debugservers/gdb/gdbserverproviderprocess.cpp debugservers/gdb/gdbserverproviderprocess.h
    debugservers/gdb/openocdgdbserverprovider.cpp debugservers/gdb/openocdgdbserverprovider.h
    debugservers/gdb/stlinkutilgdbserverprovider.cpp debugservers/gdb/stlinkutilgdbserverprovider.h
    debugservers/gdb/jlinkgdbserverprovider.cpp debugservers/gdb/jlinkgdbserverprovider.h
    iarewparser.cpp iarewparser.h
    iarewtoolchain.cpp iarewtoolchain.h
    idebugserverprovider.cpp idebugserverprovider.h
    keilparser.cpp keilparser.h
    keiltoolchain.cpp keiltoolchain.h
    sdccparser.cpp sdccparser.h
    sdcctoolchain.cpp sdcctoolchain.h
)
