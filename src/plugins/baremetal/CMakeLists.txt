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
    defaultgdbserverprovider.cpp defaultgdbserverprovider.h
    gdbserverprovider.cpp gdbserverprovider.h
    gdbserverproviderchooser.cpp gdbserverproviderchooser.h
    gdbserverprovidermanager.cpp gdbserverprovidermanager.h
    gdbserverproviderprocess.cpp gdbserverproviderprocess.h
    gdbserverproviderssettingspage.cpp gdbserverproviderssettingspage.h
    iarewparser.cpp iarewparser.h
    iarewtoolchain.cpp iarewtoolchain.h
    keilparser.cpp keilparser.h
    keiltoolchain.cpp keiltoolchain.h
    sdccparser.cpp sdccparser.h
    sdcctoolchain.cpp sdcctoolchain.h
    openocdgdbserverprovider.cpp openocdgdbserverprovider.h
    stlinkutilgdbserverprovider.cpp stlinkutilgdbserverprovider.h
)
