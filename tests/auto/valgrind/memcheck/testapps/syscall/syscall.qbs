import qbs
import "../testapp.qbs" as TestApp

TestApp { testName: "syscall"; cpp.cxxFlags: base.concat(["-O0"]); }
