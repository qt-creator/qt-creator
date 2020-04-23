add_qtc_plugin(BareMetal
  PLUGIN_DEPENDS Core Debugger ProjectExplorer
  SOURCES
    baremetal.qrc
    baremetalconstants.h
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
    debugservers/gdb/gdbserverprovider.cpp debugservers/gdb/gdbserverprovider.h
    debugservers/gdb/openocdgdbserverprovider.cpp debugservers/gdb/openocdgdbserverprovider.h
    debugservers/gdb/stlinkutilgdbserverprovider.cpp debugservers/gdb/stlinkutilgdbserverprovider.h
    debugservers/gdb/jlinkgdbserverprovider.cpp debugservers/gdb/jlinkgdbserverprovider.h
    debugservers/gdb/eblinkgdbserverprovider.cpp debugservers/gdb/eblinkgdbserverprovider.h
    debugservers/uvsc/simulatoruvscserverprovider.cpp  debugservers/uvsc/simulatoruvscserverprovider.h
    debugservers/uvsc/stlinkuvscserverprovider.cpp debugservers/uvsc/stlinkuvscserverprovider.h
    debugservers/uvsc/jlinkuvscserverprovider.cpp debugservers/uvsc/jlinkuvscserverprovider.h
    debugservers/uvsc/uvproject.cpp debugservers/uvsc/uvproject.h
    debugservers/uvsc/uvprojectwriter.cpp debugservers/uvsc/uvprojectwriter.h
    debugservers/uvsc/uvscserverprovider.cpp debugservers/uvsc/uvscserverprovider.h
    debugservers/uvsc/uvtargetdevicemodel.cpp debugservers/uvsc/uvtargetdevicemodel.h
    debugservers/uvsc/uvtargetdeviceselection.cpp debugservers/uvsc/uvtargetdeviceselection.h
    debugservers/uvsc/uvtargetdeviceviewer.cpp debugservers/uvsc/uvtargetdeviceviewer.h
    debugservers/uvsc/uvtargetdrivermodel.cpp debugservers/uvsc/uvtargetdrivermodel.h
    debugservers/uvsc/uvtargetdriverselection.cpp debugservers/uvsc/uvtargetdriverselection.h
    debugservers/uvsc/uvtargetdriverviewer.cpp debugservers/uvsc/uvtargetdriverviewer.h
    debugservers/uvsc/xmlnodevisitor.h
    debugservers/uvsc/xmlproject.cpp debugservers/uvsc/xmlproject.h
    debugservers/uvsc/xmlprojectwriter.cpp debugservers/uvsc/xmlprojectwriter.h
    debugservers/uvsc/xmlproperty.cpp debugservers/uvsc/xmlproperty.h
    debugservers/uvsc/xmlpropertygroup.cpp debugservers/uvsc/xmlpropertygroup.h
    iarewparser.cpp iarewparser.h
    iarewtoolchain.cpp iarewtoolchain.h
    idebugserverprovider.cpp idebugserverprovider.h
    keilparser.cpp keilparser.h
    keiltoolchain.cpp keiltoolchain.h
    sdccparser.cpp sdccparser.h
    sdcctoolchain.cpp sdcctoolchain.h
)
