# Syntax Highlighting

Syntax highlighting engine for Kate syntax definitions

## Table of contents

1. [Introduction](#introduction)
2. [Out of scope](#out-of-scope)
3. [Syntax definition files](#syntax-definition-files)
4. [Color theme files](#color-theme-files)
5. [Build it](#build-it)
6. [How to contribute](#how-to-contribute)
   * [Licensing](#licensing)
   * [Tips for contributing to syntax definition files](#tips-for-contributing-to-syntax-definition-files)
   * [Adding unit tests for a syntax definition](#adding-unit-tests-for-a-syntax-definition)
7. [Report bug or help to fix them](#report-bug-or-help-to-fix-them)
8. [Updating the syntax & themes pages of the kate-editor.org website](#updating-the-kate-editororgsyntax-website)

## Introduction

This is a stand-alone implementation of the Kate syntax highlighting engine.
It's meant as a building block for text editors as well as for simple highlighted
text rendering (e.g. as HTML), supporting both integration with a custom editor
as well as a ready-to-use QSyntaxHighlighter sub-class.

Besides a C++ API, a [QML API](@ref qml_api) is also provided.

## Out of scope

To not turn this into yet another text editor, the following things are considered
out of scope:

* code folding, beyond providing folding range information
* auto completion
* spell checking
* user interface for configuration
* management of text buffers or documents

If you need any of this, check out [KTextEditor](https://api.kde.org/frameworks/ktexteditor/html/).

## Syntax definition files

This library uses Kate syntax definition files for the actual highlighting,
the file format is documented [here](https://docs.kde.org/?application=katepart&branch=trunk5&path=highlight.html).

More than 300 syntax definition files are included, that are located
in **data/syntax/** and have the **.xml** extension. Additional ones are
picked up from the file system if present, so you can easily extend this
by application-specific syntax definitions for example.

To install or test a syntax definition file locally, place it in
**org.kde.syntax-highlighting/syntax/**, which is located in your user directory.
Usually it is:

<table>
    <tr>
        <td>For local user</td>
        <td>$HOME/.local/share/org.kde.syntax-highlighting/syntax/</td>
    </tr>
    <tr>
        <td>For Flatpak packages</td>
        <td>$HOME/.var/app/<em>package-name</em>/data/org.kde.syntax-highlighting/syntax/</td>
    </tr>
    <tr>
        <td>For Snap packages</a></td>
        <td>$HOME/snap/<em>package-name</em>/current/.local/share/org.kde.syntax-highlighting/syntax/</td>
    </tr>
    <tr>
        <td>On Windows®</td>
        <td>&#37;USERPROFILE&#37;&#92;AppData&#92;Local&#92;org.kde.syntax-highlighting&#92;syntax&#92;</td>
    </tr>
    <tr>
        <td>On macOS®</td>
        <td>$HOME/Library/Application Support/org.kde.syntax-highlighting/syntax/</td>
    </tr>
</table>

For more details, see ["The Highlight Definition XML Format" (Working with Syntax Highlighting, KDE Documentation)](https://docs.kde.org/?application=katepart&branch=trunk5&path=highlight.html#katehighlight-xml-format).

Also, in **data/schema/** there is a script to validate the syntax definition XML
files. Use the command `validatehl.sh mySyntax.xml`.

## Color theme files

This library includes the color themes, which are documented
[here](https://docs.kde.org/?application=katepart&branch=trunk5&path=color-themes.html).

The color theme files use the JSON format and are located in **data/themes/**
with the **.theme** extension.
Additional ones are also picked up from the file system if present,
in the **org.kde.syntax-highlighting/themes/** folder of your user directory,
allowing you to easily add custom color theme files. This location is the same
as shown in the table of the [previous section](#syntax-definition-files),
replacing the **syntax** folder with **themes**.
For more details, see ["The Color Themes JSON Format" (Working with Color Themes, KDE Documentation)](https://docs.kde.org/?application=katepart&branch=trunk5&path=color-themes.html#color-themes-json).

The [KTextEditor](https://api.kde.org/frameworks/ktexteditor/html/) library
(used by Kate, Kile and KDevelop, for example) provides a
[user interface](https://docs.kde.org/?application=katepart&branch=trunk5&path=color-themes.html#color-themes-gui)
for editing and creating KSyntaxHighlighting color themes, including
a tool for exporting and importing the JSON theme files.

Note that in KDE text editors, the KSyntaxHighlighting color themes are used
[since KDE Frameworks 5.75](https://kate-editor.org/post/2020/2020-09-13-kate-color-themes-5.75/),
released on October 10, 2020. Previously, Kate's color schemes
(KConfig based schema config) were used and are now deprecated.
The tool **utils/schema-converter/** and the script **utils/kateschema_to_theme_converter.py**
convert the old Kate schemas to KSyntaxHighlighting themes.

Also see ["Submit a KSyntaxHighlighting Color Theme" (Kate Editor Website)](https://kate-editor.org/post/2020/2020-09-18-submit-a-ksyntaxhighlighting-color-theme/).

## Build it

1. Create and change into a build directory. Usually, a folder called **build**
   is created inside the **syntax-highlighting** source directory.

   ```bash
   mkdir <build-directory>
   cd <build-directory>
   ```

2. Run the configure process with *cmake* and compile:

   ```bash
   cmake <source-directory>
   make
   ```

   For example:

   ```bash
   git clone git@invent.kde.org:frameworks/syntax-highlighting.git
   mkdir ./syntax-highlighting/build
   cd ./syntax-highlighting/build
   cmake ../
   make
   ```

   For more details see ["Building Kate from Sources on Linux" (Kate Editor Website)](https://kate-editor.org/build-it/).

   **NOTE:** If running *cmake* shows an error related to your version of KDE
   Frameworks, you edit the **CMakeLists.txt** file in the line
   `find_package(ECM 5.XX.X ...)`.

3. To run tests:

   ```bash
   make test
   ```

   The tests are located in the **autotests** directory.
   This command can be used to check changes to units test after modifying some
   syntax definition file. To add a unit test or update the references, see the
   section ["Adding unit tests for a syntax definition"](#adding-unit-tests-for-a-syntax-definition).

## How to contribute

KDE uses a GitLab instance at **invent.kde.org** for code review. The official
repository of the KSyntaxHighlighting framework is [here](https://invent.kde.org/frameworks/syntax-highlighting).

All the necessary information to send contributions is [here](https://community.kde.org/Infrastructure/GitLab).

### Licensing

Contributions to KSyntaxHighlighting shall be licensed under [MIT](LICENSES/MIT.txt).

All files shall contain a proper "SPDX-License-Identifier: MIT" identifier inside a header like:

```cpp
/*
    SPDX-FileCopyrightText: 2020 Christoph Cullmann <cullmann@kde.org>

    SPDX-License-Identifier: MIT
*/
```

### Tips for contributing to syntax definition files

* If you are modifying an existing syntax definition XML file, you must increase
  the version number of the language.

* Do not use hard-coded colors, as they may not look good or be illegible in some color
  themes. Prefer to use the default color styles.

  For more information, see:

    * [Available Default Styles (Working with Syntax Highlighting, KDE Documentation)](https://docs.kde.org/?application=katepart&branch=trunk5&path=highlight.html#kate-highlight-default-styles)
    * [Kate Part (KF5): New Default Styles for better Color Schemes (Kate Editor Website)](https://kate-editor.org/2014/03/07/kate-part-kf5-new-default-styles-for-better-color-schemes/)

* Add test files, these are found in **autotests/input/**.
  If you are going to add a new syntax XML file, create a new test file; if you
  are going to modify a XML file, adds examples to existing test files.

  Then, it is necessary to generate and update the files in **autotests/folding/**,
  **autotests/html/** and **autotests/reference/**, which must be included in the
  patches. Instructions are [below](#adding-unit-tests-for-a-syntax-definition).

### Adding unit tests for a syntax definition

1. Add an input file into the **autotests/input/** folder, lets call it
   **test.&lt;language-extension&gt;**.

2. If the file extension is not sufficient to trigger the right syntax definition, you can add an
   second file **testname.&lt;language-extension&gt;.syntax** that contains the syntax definition name
   to enforce the use of the right extension.

3. Do `make && make test`.

   Note that after adding or modifying something in
   **&lt;source-directory&gt;/autotests/input/**, an error will be showed when
   running `make test`, because the references in the source directory do not
   match the ones now generated.

4. Inspect the outputs found in your binary directory **autotests/folding.out/**,
   **autotests/html.output/** and **autotests/output/**.

5. If OK, run in the binary folder `./autotests/update-reference-data.sh`
   to copy the results to the right location.
   That script updates the references in the source directory in
   **autotest/folding/**, **autotest/html/** and **autotest/reference/**.

6. Add the result references after the copying to the git.

## Report bug or help to fix them

KDE uses Bugzilla to management of bugs at **bugs.kde.org**. You can see the bugs
reported of **frameworks-syntax-highlighting** [here](https://bugs.kde.org/describecomponents.cgi?product=frameworks-syntax-highlighting).

Also, you can report a bug [here](https://bugs.kde.org/enter_bug.cgi?product=frameworks-syntax-highlighting).

However, some users often report bugs related to syntax highlighting in
[kate/syntax](https://bugs.kde.org/buglist.cgi?component=syntax&product=kate&resolution=---)
and [kile/editor](https://bugs.kde.org/buglist.cgi?component=editor&product=kile&resolution=---).

## Updating the syntax & themes pages of the kate-editor.org website

To update the [kate-editor.org/syntax](https://kate-editor.org/syntax/) and
[kate-editor.org/themes](https://kate-editor.org/themes/) websites
including the update site & all linked examples/files,
please run after successful **build** & **test** the following make target:

```bash
make update_kate_editor_org
```

This will clone the [kate-editor.org git](https://invent.kde.org/websites/kate-editor-org)
from *invent.kde.org* into **kate-editor-org** inside the build directory and update the needed things.

You can afterwards step into **kate-editor-org** and commit & push the change after review.

The [kate-editor.org](https://kate-editor.org) webserver will update itself periodically from the repository on *invent.kde.org*.
