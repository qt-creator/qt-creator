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

The Qt Design Studio Manual is based on the Qt Creator Manual, with
additional topics. For more information, see the `README` file in the
qtdesignstudio subfolder.

The Extending Qt Creator Manual has its own sources. In addition, it
pulls in API reference documentation from the Qt Creator source files.

# QDoc Warnings

All the documents are built when you enter `make docs` on Linux or
macOS or `nmake docs` on Windows. At the time of this writing, this
leads to QDoc warnings being generated, because the Qt Creator Manual
requires QDoc from Qt 5.14 or later (it links to new modules), whereas
the Extending Qt Creator Manual requires QDoc from Qt 5.10 or earlier,
because the doc configuration is not supported when using the Clang
parser.

To hide the doc errors and make doc builds faster, enter an option
to write the doc errors to the log. For example, on Windows enter
`nmake docs 2> log.txt`.
