# Documentation projects in this repository

The qtcreator repository contains the sources for building the following
documents:

- [Qt Creator Documentation](https://doc.qt.io/qtcreator/)
- [Extending Qt Creator Manual](https://doc.qt.io/qtcreator-extending/)
- [Qt Design Studio documentation](https://doc.qt.io/qtdesignstudio/)

The sources for each project are stored in the following subfolders of
the doc folder:

- qtcreator
- qtcreatordev
- qtdesignstudio

For more information, see:
[Writing Documentation](https://doc.qt.io/qtcreator-extending/qtcreator-documentation.html)

The Qt Design Studio documentation is based on the Qt Creator documentation,
with additional topics. For more information, see the `README` file in the
qtdesignstudio subfolder.

The Extending Qt Creator Manual has its own sources. In addition, it
pulls in API reference documentation from the Qt Creator source files.

**Note:** Please build the docs before submitting code changes to make sure that
you do not introduce new QDoc warnings.
