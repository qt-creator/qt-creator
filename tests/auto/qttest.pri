include(../../qtcreator.pri)
include(qttestrpath.pri)

isEmpty(TEMPLATE):TEMPLATE=app
CONFIG += qt warn_on console depend_includepath testcase qtestlib
CONFIG -= app_bundle

symbian:{
    TARGET.EPOCHEAPSIZE = 0x100000 0x2000000
#    DEFINES += QTEST_NO_SPECIALIZATIONS
    TARGET.CAPABILITY="None"
    RSS_RULES ="group_name=\"QtTests\";"
}

# prefix test binary with tst_
!contains(TARGET, ^tst_.*):TARGET = $$join(TARGET,,"tst_")

win32 {
    lib = $$IDE_LIBRARY_PATH;$$IDE_PLUGIN_PATH/Nokia
    lib ~= s,/,\\,g
    # the below gets added to later by testcase.prf
    check.commands = cd . & set PATH=$$lib;%PATH%& cmd /c
}
