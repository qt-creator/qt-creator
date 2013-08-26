import qbs
import "../testapp.qbs" as TestApp

TestApp { testName: "uninit2"; cpp.cxxFlags: base.concat(["-O0"]); }
