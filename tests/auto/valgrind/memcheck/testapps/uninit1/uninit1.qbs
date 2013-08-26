import qbs
import "../testapp.qbs" as TestApp

TestApp { testName: "uninit1"; cpp.cxxFlags: base.concat(["-O0"]); }
