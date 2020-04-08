# Documentation Projects in This Repository

The qtcreator repository contains the sources for building the following
documents:

- Qt Creator Manual
- Extending Qt Creator Manual
- Qt Design Studio Manual

The sources for each project are stored in the following subfolders of
the doc folder:

- qtcreator
- qtcreatordev
- qtdesignstudio

For more information, see:
[Writing Documentation](https://doc.qt.io/qtcreator-extending/qtcreator-documentation.html)

The Qt Design Studio Manual is based on the Qt Creator Manual, with
additional topics. For more information, see the `README` file in the
qtdesignstudio subfolder.

The Extending Qt Creator Manual has its own sources. In addition, it
pulls in API reference documentation from the Qt Creator source files.

# QDoc

All the documents are built when you enter `make docs` on Linux or
macOS or `nmake docs` on Windows.

Since Qt Creator 4.12, you need to use QDoc Qt 5.14 or later to build
the docs. While building with QDoc from Qt 5.11 or later technically
works, the Qt Creator Manual and Qt Design Studio Manual link to newer
Qt modules, which means link errors will be printed.

Please make the docs before submitting code changes to make sure that
you do not introduce new QDoc warnings.

While working on changes that introduce lots of warnings about missing API
documentation, for example, you can enter an option to write the doc
errors to the log. This helps make doc builds faster until you have
fixed the errors. For example, on Windows enter `nmake docs 2> log.txt`.
