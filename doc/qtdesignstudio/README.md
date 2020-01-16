# Writing Qt Design Studio Documentation

The Qt Design Studio Manual is based on the Qt Creator Manual, with additional
topics. When building the Qt Design Studio Manual, parts of the Qt Creator
Manual are pulled in from Qt Creator sources. This is enabled by creating
separate table of contents files for each Manual and by using defines
to hide and show information depending on which Manual is being built.

This readme file describes how to build the Qt Design Studio Manual and how to
edit or add source files when necessary.

## Building the Qt Design Studio Manual

1. Edit qtcreator\qtcreator.pri as follows:
   `isEmpty(IDE_DISPLAY_NAME):  IDE_DISPLAY_NAME = Qt Design Studio`
   `isEmpty(IDE_ID):            IDE_ID = qtdesignstudio`
   `isEmpty(IDE_CASED_ID):      IDE_CASED_ID = QtDesignStudio`
2. Switch to the `qtcreator\doc\qtdesignstudio` directory.
4. Run `qmake` from Qt 5.14.0, or later
  (because you need the Qt Timeline and Qt Quick 3D module docs)
5. Run `make docs` on Linux and macOS or `nmake docs` on Windows.

The docs are generated in `qtcreator\doc\qtdesignstudio\doc\qtdesignstudio`

## Showing and Hiding Information

Qt Design Studio uses only a subset of Qt Creator plugins and it has its
own special plugins. This means that their manuals have somewhat different
structures. Which, in turn breaks the navigation links to previous and next
pages.

This also means that some of the Qt Creator doc source files are not needed at
all and some contain information that does not apply to the Qt Design Studio
Manual. If QDoc parses all the Qt Creator doc sources, it would generate
HTML files for each topic and include those files and all the images that they
refer to in the help compilation files. This would unnecessarily increase the
size of the help database and pollute the help index with references to files
that are not actually listed in the table of contents of the manual. To avoid
this, the files that are not needed are excluded from doc builds in the doc
build configuration file.

### Fixing the Navigation Links

The navigation order of the topics in the Qt Design Studio Manual is specified
in `qtcreator\doc\qtdesignstudio\src\qtdesignstudio-toc.qdoc`. The
order of the topics in the Qt Creator Manual is specified in
`qtcreator\doc\src\qtcreator-toc.qdoc`. If you add topics to or move them
around in the TOC file, you must adjust the navigation links accordingly.

The `qtdesignstudio` define is specified as a value of the `defines` option in
the Qt Design Studio doc configuration file,
`qtcreator\doc\qtdesignstudio\config\qtdesignstudio.qdocconf`. It is
mostly used in the Qt Creator doc sources to specify values for the
\previouspage and \nextpage commands depending on whether the Qt Design Studio
Manual or Qt Creator Manual is being built. For example, the following if-else
statement is needed, because the `quick-buttons.html` is excluded from the
Qt Design Studio Manual:

`\page quick-components.html`
`\previouspage creator-using-qt-quick-designer.html`
`\if defined(qtdesignstudio)`
`\nextpage qtquick-navigator.html`
`\else`
`\nextpage quick-buttons.html`
`\endif`

### Excluding Souce Files from Builds

The directories to exclude from Qt Design Studio Manual builds are listed as
values of the `excludedirs` option in `\config\qtdesignstudio.qdocconf`.

You only need to edit the values of the option if you want to show or hide all
the contents of a directory. For example, if you add support for a Qt Creator
plugin that was previously not supported by Qt Design Studio, you should remove
the directory that contains the documentation for the plugin from the values.

To hide or show individual topics within individual `.qdoc` files, you need to
move the files in the Qt Creator doc source (`qtcreator\doc\src`) to or from
excluded directories.

For example, if support for iOS were added, you would need to check whether the
information about iOS support is applicable to Qt Design Studio Manual. If yes,
you would need to remove the following line from the `excludedirs` value:

`../../src/ios \`

You would then use defines to hide any Qt Creator specific information from the
source file in the directory.

If a directory contains some files that are needed in both manuals and some that
are only needed in the Qt Creator Manual, the latter are located in a
subdirectory called `creator-only`, which is excluded from the Qt Design Studio
doc builds.

### Hiding Text in Qt Creator Doc Sources

The `qtcreator` define is specified as a value of the `defines` option in the
Qt Creator doc configuration file,
`qtcreator\doc\src\config\qtcreator-project.qdocconf`. It is mostly used in the
Qt Creator doc sources to hide Qt Creator specific information when the Qt
Design Studio Manual is built.

The `\else` command is sometimes used to replace some Qt Creator specific text
with text that applies to Qt Design Studio. For example, the following if-else
statement is needed in the Qt Creator doc sources, because the project wizards
in Qt Design Studio are different from those in Qt Creator, and are therefore
described in a new topic that is located in the Qt Design Studio doc sources:

`For more information, see`
`\if defined(qtcreator)`
`\l{Creating Qt Quick Projects}.`
`\else`
`\l{Creating UI Prototype Projects}.`
`\endif`

Note that titles in the two manuals can be identical only if the page is
excluded from the Qt Design Studio Manual. In this case, QDoc can correctly
determine the link target. If you add a link to a section title that appears
twice in the doc source files, QDoc uses the first reference to that title
in the `.index` file.

## Writing About Qt Design Studio Specific Features

Qt Design Studio specific plugins and features are described in a set of doc
source files located in the `qtcreator\doc\qtdesignstudio\src` directory. Some
files are used to include subsections in topics in the Qt Creator doc sources.

Screenshots and other illustrations are stored in the `\qtdesignstudio\images`
directory.

### Adding Topics

If you add new topics to the Qt Design Studio Manual, add links to them to the
table of contents in `qtdesignstudio-toc.qdoc` and check the values of the
navigation links around them.

### Including Sections in Qt Creator Doc Sources

Qt Quick Designer is an integral part of both Qt Creator and Qt Design Studio.
Therefore, most topics that describe it are needed in the manuals of both tools.
You can use the `\include` command in the Qt Creator doc sources to include
`.qdocinc` files from the Qt Design Studio doc sources when building the
Qt Design Studio Manual.

For example, the following lines in the
`qtcreator\doc\src\qtquick\qtquick-components.qdoc` file add information about
creating and using Qt Design Studio Components to the `Creating Components`
topic that is pulled from the Qt Creator doc sources:

`\if defined(qtdesignstudio)`
`\include qtdesignstudio-components.qdocinc creating studio components`
`\include qtdesignstudio-components.qdocinc studio components`
`\endif`

Similarly, you can use include files to include subsections in different main
level topics in the two manuals.
