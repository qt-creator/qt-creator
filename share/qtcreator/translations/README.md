How To add translations to Qt Creator
=====================================

- Coordinate over the mailing list to avoid duplicate work.

- Read the instructions at http://wiki.qt.io/Qt_Localization

- Add your language to the `set(languages ...` line in `CMakeLists.txt`. Don't
  qualify it with a country unless it is reasonable to expect country-specific
  variants. Skip this step if updating an existing translation.

- Configure a Qt Creator build directory with CMake.

- Run `cmake --build . --target ts_<lang>`.

  If your Qt version is too old, you may create a template by running `lconvert
  --drop-translations qtcreator_de.ts -o qtcreator_<yours>.ts` and adjusting the
  language attribute in the new .ts file. The downside of this method is
  that you will not be translating the newest Creator sources (unless
  qtcreator_de.ts was completely up-to-date). Of course, this method is not
  applicable if you are updating an existing translation.

  You may also request an up-to-date template from us.

- Start Qt Linguist and translate the strings.

- Create a commit:
  - Discard the modifications to any `.ts` files except yours
  - Create a commit with your file
  - If needed, amend the commit with the modified `CMakeLists.txt` file

- Follow http://wiki.qt.io/Qt_Contribution_Guidelines to post the change for
  review.

- .qm files are generated as part of the regular build.

_Note:_ QmlDesigner contains code from the Gradient Editor of Qt Designer. If an
official translation of Qt for your language exists, you can re-use the
translation of those messages by merging Qt Creator's and Qt Designer's
translation using `lconvert`:

    lconvert qtcreator_<LANG>.ts $QTDIR/translations/designer_<LANG>.ts > temp.ts

Move the temporary file back to `qtcreator_<LANG>.ts`, complete the Gradient
Editor's translations and update the file, passing the additional option
`-noobsolete` to `lupdate` (by temporarily modifying
`<qtcreator>/cmake/QtCreatorTranslations.cmake`). This will remove the now
redundant messages originating from Qt Designer.
