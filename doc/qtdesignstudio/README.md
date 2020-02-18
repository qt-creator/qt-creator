# Writing Qt Design Studio Documentation

The Qt Design Studio Manual is based on the Qt Creator Manual, with additional
topics. When building the Qt Design Studio Manual, parts of the Qt Creator
Manual are pulled in from Qt Creator sources. This is enabled by creating
separate table of contents files for each Manual and by using defines
to hide and show information depending on which Manual is being built.

Because branding information is needed to use the correct product name and
version, you must run `qmake -r` on `qtcreator.pro` with the `IDE_BRANDING_PRI`
option set to the absolute path of `ide_branding.pri` in the Qt Design Studio
repository.

For example, on Windows enter (all on one line):

`C:\dev\qtc-super\qtcreator>..\..\..\Qt\5.14.1\msvc2017_64\bin\qmake.exe
    qtcreator.pro -r
    IDE_BRANDING_PRI=C:\dev\tqtc-plugin-qtquickdesigner\studiodata\branding\ide_branding.pri`

## Building the Qt Design Studio Manual

1. Run `qmake` from Qt 5.14.0, or later with the path to the branding
   information as a parameter:
   `<relative_path_to>\qmake.exe qtcreator.pro -r IDE_BRANDING_PRI=<absolute_path_to>\tqtc-plugin-qtquickdesigner\studiodata\branding\ide_branding.pri`
5. Run `make docs` on Linux and macOS or `nmake docs` on Windows.

The docs are generated in `qtcreator\doc\html\qtdesignstudio` with the
Qt Design Studio branding.
