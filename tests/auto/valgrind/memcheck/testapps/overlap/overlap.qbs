import qbs
import "../testapp.qbs" as TestApp

TestApp { testName: "overlap"; cpp.cxxFlags: base.concat(["-O0", "-fno-builtin"]); }
