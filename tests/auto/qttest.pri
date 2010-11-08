include(../../qtcreator.pri)

isEmpty(TEMPLATE):TEMPLATE=app
CONFIG += qt warn_on console depend_includepath testcase qtestlib
CONFIG -= app_bundle

symbian:{
    TARGET.EPOCHEAPSIZE = 0x100000 0x2000000
#    DEFINES += QTEST_NO_SPECIALIZATIONS
    TARGET.CAPABILITY="None"
    RSS_RULES ="group_name=\"QtTests\";"
}

include(qttestrpath.pri)

# prefix test binary with tst_
!contains(TARGET, ^tst_.*):TARGET = $$join(TARGET,,"tst_")
