
The important test here is tst_dumpers.

The same build can be used to check the dumpers under different
conditions by using environment variables as follows:

   QTC_DEBUGGER_PATH_FOR_TEST - path to a GDB or LLDB binary

   QTC_QMAKE_PATH_FOR_TEST - path to a Qt version

   QTC_MAKE_PATH_FOR_TEST - path to a "make".
      Used for GDB only, defaults to "make" except on Windows,
      where it defaults to "mingw32-make"

   QTC_USE_GLIBCXXDEBUG_FOR_TEST - (0/1) to switch between GCC's
      "normal" standard library, and the "debug" version
      (this will add DEFINES += _GLIBCXX_DEBUG) to the .pro

   QTC_BOOST_INCLUDE_PATH_FOR_TEST - include path for boost libraries
      only necessary if you have more than one version in different
      paths installed or if a non-standard path has been used

  (QTC_MSVC_ENV_BAT - to set up MSVC)
  (QTC_CDBEXT_PATH (optional) - path to the cdbextension
      defaults to IDE_BUILD_TREE/IDE_LIBRARY_BASENAME/qtcreatorcdbext64)

The tests should be used for automated testing, but can also
be used for dumper development and fixing.

Combinations that should always succeed are:

   GDB (>=7.4), Linux (32/64), Python (2/3), Qt (4/5) - debug.

Partially work should:

   Qt (4/5) - release
   LLDB (Mac)


By default, only successful tests are cleaned up (use
QTC_KEEP_TEMP_FOR_TEST to override).

Failing tests leave a qt_tst_dumpers_XXXXXX directory behind,
with a 'doit.pro' which can be directly opened in Creator.
There's always a 'main.cpp' file, containing at least one
call to a function 'breakHere()'. Put a break point there,
and hit F5.

The file 'input.txt' contains the commands sent to GDB
in case something needs to be examined with the CLI debugger.
