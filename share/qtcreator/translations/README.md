How To add translations to Qt Creator
=====================================

- Coordinate over the [mailing list][1] to avoid duplicate work.

- Read the instructions at http://wiki.qt.io/Qt_Localization

## Preparation

- It is preferred that you work on the current stable branch. That usually is
  the branch with the same major and minor version number as the most recent
  stable release (not beta or RC) of Qt Creator.

- Configure a Qt Creator build directory with CMake.

## Adding a new language

- Create a new translation file by running

      cmake --build . -target ts_untranslated

  in the build directory. This generates a file

      share/qtcreator/translations/qtcreator_untranslated.ts

  in the source directory.

- Rename the file `qtcreator_untranslated.ts` to `qtcreator_<lang>.ts`. (Where
  `<lang>` is the language specifier for the language you add. Don't qualify it
  with a country unless it is reasonable to expect country-specific variants.)

- Add `<lang>` to the `set(languages ...` line in `CMakeLists.txt` in
  this directory.

## Updating translations

- Run

      cmake --build . --target ts_<lang>

  in the build directory. This updates the `.ts` file for the language `<lang>`
  in the source directory with the new and changed translatable strings.

  It includes entries for obsolete (= removed) items and the source locations
  of the translatable strings. These can be useful during the translation
  process, but need to be removed before creating the commit (see below).
  If you do not need or want this information during the translation process,
  you can also directly run

      cmake --build . --target ts_<lang>_cleaned

  instead of the `ts_<lang>` target.

- Start Qt Linguist and translate the strings.

  Start with translations in contexts starting with "QtC::". These are the most
  essential ones.

  Do not forget to "accept" finalized translations with the "Done" or "Done and
  Next" action in Linguist, to remove the `untranslated` state from translations
  before submitting.

- Prepare for committing. You need to remove the obsolete items and location
  information before the file can be submitted to the source repository. Do this
  by running

      cmake --build . --target ts_<lang>_cleaned

  in the build directory.

- Create a commit:
    - Discard the modifications to any `.ts` files except yours
    - Create a commit with your file
    - If you added a new language, amend the commit with the modified
      `CMakeLists.txt` file

- Follow http://wiki.qt.io/Qt_Contribution_Guidelines to post the change for
  review.

*Note:* `.qm` files are generated as part of the regular build. They are not
submitted to the repository.

*Note:* QmlDesigner contains code from the Gradient Editor of Qt Designer. If an
official translation of Qt for your language exists, you can re-use the
translation of those messages by merging Qt Creator's and Qt Designer's
translation using `lconvert`:

    lconvert qtcreator_<LANG>.ts $QTDIR/translations/designer_<LANG>.ts > temp.ts

Move the temporary file back to `qtcreator_<LANG>.ts`, complete the Gradient
Editor's translations and update the file, passing the additional option
`-noobsolete` to `lupdate` (by temporarily modifying
`<qtcreator>/cmake/QtCreatorTranslations.cmake`). This will remove the now
redundant messages originating from Qt Designer.

[1]: https://lists.qt-project.org/listinfo/localization
