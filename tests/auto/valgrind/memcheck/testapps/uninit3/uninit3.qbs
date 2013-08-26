import qbs
import "../testapp.qbs" as TestApp

TestApp { testName: "uninit3"; cpp.cxxFlags: base.concat(["-O0"]); }
