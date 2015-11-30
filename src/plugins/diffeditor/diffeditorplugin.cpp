/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "diffeditorplugin.h"
#include "diffeditor.h"
#include "diffeditorconstants.h"
#include "diffeditorcontroller.h"
#include "diffeditordocument.h"
#include "diffeditorfactory.h"
#include "differ.h"

#include <QAction>
#include <QFileDialog>
#include <QTextCodec>
#include <QtPlugin>

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>

#include <utils/qtcassert.h>

namespace DiffEditor {
namespace Internal {

class FileDiffController : public DiffEditorController
{
    Q_OBJECT
public:
    FileDiffController(Core::IDocument *document, const QString &leftFileName,
                       const QString &rightFileName);

protected:
    void reload();

private:
    QString m_leftFileName;
    QString m_rightFileName;
};

FileDiffController::FileDiffController(Core::IDocument *document, const QString &leftFileName,
                                       const QString &rightFileName) :
    DiffEditorController(document), m_leftFileName(leftFileName), m_rightFileName(rightFileName)
{ }

void FileDiffController::reload()
{
    QString errorString;
    Utils::TextFileFormat format;
    format.codec = Core::EditorManager::defaultTextCodec();

    QString leftText;
    if (Utils::TextFileFormat::readFile(m_leftFileName,
                                    format.codec,
                                    &leftText, &format, &errorString)
            != Utils::TextFileFormat::ReadSuccess) {
        return;
    }

    QString rightText;
    if (Utils::TextFileFormat::readFile(m_rightFileName,
                                    format.codec,
                                    &rightText, &format, &errorString)
            != Utils::TextFileFormat::ReadSuccess) {
        return;
    }

    Differ differ;
    QList<Diff> diffList = differ.cleanupSemantics(differ.diff(leftText, rightText));

    QList<Diff> leftDiffList;
    QList<Diff> rightDiffList;
    Differ::splitDiffList(diffList, &leftDiffList, &rightDiffList);
    QList<Diff> outputLeftDiffList;
    QList<Diff> outputRightDiffList;

    if (ignoreWhitespace()) {
        const QList<Diff> leftIntermediate
                = Differ::moveWhitespaceIntoEqualities(leftDiffList);
        const QList<Diff> rightIntermediate
                = Differ::moveWhitespaceIntoEqualities(rightDiffList);
        Differ::ignoreWhitespaceBetweenEqualities(leftIntermediate, rightIntermediate,
                                                  &outputLeftDiffList, &outputRightDiffList);
    } else {
        outputLeftDiffList = leftDiffList;
        outputRightDiffList = rightDiffList;
    }

    const ChunkData chunkData = DiffUtils::calculateOriginalData(
                outputLeftDiffList, outputRightDiffList);
    FileData fileData = DiffUtils::calculateContextData(chunkData, contextLineCount(), 0);
    fileData.leftFileInfo.fileName = m_leftFileName;
    fileData.rightFileInfo.fileName = m_rightFileName;

    QList<FileData> fileDataList;
    fileDataList << fileData;

    setDiffFiles(fileDataList);
    reloadFinished(true);
}

/////////////////

bool DiffEditorPlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorMessage)

    //register actions
    Core::ActionContainer *toolsContainer
            = Core::ActionManager::actionContainer(Core::Constants::M_TOOLS);
    toolsContainer->insertGroup(Core::Constants::G_TOOLS_OPTIONS, Constants::G_TOOLS_DIFF);

    QAction *diffAction = new QAction(tr("Diff..."), this);
    Core::Command *diffCommand = Core::ActionManager::registerAction(diffAction, "DiffEditor.Diff");
    connect(diffAction, &QAction::triggered, this, &DiffEditorPlugin::diff);
    toolsContainer->addAction(diffCommand, Constants::G_TOOLS_DIFF);

    addAutoReleasedObject(new DiffEditorFactory(this));

    return true;
}

void DiffEditorPlugin::extensionsInitialized()
{ }

void DiffEditorPlugin::diff()
{
    QString fileName1 = QFileDialog::getOpenFileName(Core::ICore::dialogParent(),
                                                     tr("Select First File for Diff"),
                                                     QString());
    if (fileName1.isNull())
        return;

    QString fileName2 = QFileDialog::getOpenFileName(Core::ICore::dialogParent(),
                                                     tr("Select Second File for Diff"),
                                                     QString());
    if (fileName2.isNull())
        return;


    const QString documentId = QLatin1String("Diff ") + fileName1 + QLatin1String(", ") + fileName2;
    QString title = tr("Diff \"%1\", \"%2\"").arg(fileName1).arg(fileName2);
    auto const document = qobject_cast<DiffEditorDocument *>(
                DiffEditorController::findOrCreateDocument(documentId, title));
    if (!document)
        return;

    if (!DiffEditorController::controller(document))
        new FileDiffController(document, fileName1, fileName2);
    Core::EditorManager::activateEditorForDocument(document);
    document->reload();
}

} // namespace Internal
} // namespace DiffEditor

#ifdef WITH_TESTS

#include <QTest>

#include "diffutils.h"

Q_DECLARE_METATYPE(DiffEditor::ChunkData)
Q_DECLARE_METATYPE(DiffEditor::FileData)

static inline QString _(const char *string) { return QString::fromLatin1(string); }

void DiffEditor::Internal::DiffEditorPlugin::testMakePatch_data()
{
    QTest::addColumn<ChunkData>("sourceChunk");
    QTest::addColumn<QString>("leftFileName");
    QTest::addColumn<QString>("rightFileName");
    QTest::addColumn<bool>("lastChunk");
    QTest::addColumn<QString>("patchText");

    const QString fileName = _("a.txt");
    const QString header = _("--- ") + fileName + _("\n+++ ") + fileName + _("\n");

    QList<RowData> rows;
    rows << RowData(_("ABCD"), TextLineData::Separator);
    rows << RowData(_("EFGH"));
    ChunkData chunk;
    chunk.rows = rows;
    QString patchText = header + _("@@ -1,2 +1,1 @@\n"
                                   "-ABCD\n"
                                   " EFGH\n");
    QTest::newRow("Simple not a last chunk") << chunk
                            << fileName
                            << fileName
                            << false
                            << patchText;

    ///////////

    // chunk the same here
    patchText = header + _("@@ -1,2 +1,1 @@\n"
                           "-ABCD\n"
                           " EFGH\n"
                           "\\ No newline at end of file\n");

    QTest::newRow("Simple last chunk") << chunk
                            << fileName
                            << fileName
                            << true
                            << patchText;

    ///////////

    rows.clear();
    rows << RowData(_("ABCD"));
    rows << RowData(_(""), TextLineData::Separator);
    chunk.rows = rows;
    patchText = header + _("@@ -1,1 +1,1 @@\n"
                           "-ABCD\n"
                           "+ABCD\n"
                           "\\ No newline at end of file\n");

    QTest::newRow("EOL in last line removed") << chunk
                            << fileName
                            << fileName
                            << true
                            << patchText;

    ///////////

    // chunk the same here
    patchText = header + _("@@ -1,2 +1,1 @@\n"
                           " ABCD\n"
                           "-\n");

    QTest::newRow("Last empty line removed") << chunk
                            << fileName
                            << fileName
                            << false
                            << patchText;

    ///////////

    rows.clear();
    rows << RowData(_("ABCD"));
    rows << RowData(_(""), TextLineData::Separator);
    rows << RowData(_(""), TextLineData::Separator);
    chunk.rows = rows;
    patchText = header + _("@@ -1,2 +1,1 @@\n"
                           "-ABCD\n"
                           "-\n"
                           "+ABCD\n"
                           "\\ No newline at end of file\n");

    QTest::newRow("Two last EOLs removed") << chunk
                            << fileName
                            << fileName
                            << true
                            << patchText;

    ///////////

    rows.clear();
    rows << RowData(_("ABCD"));
    rows << RowData(TextLineData::Separator, _(""));
    chunk.rows = rows;
    patchText = header + _("@@ -1,1 +1,1 @@\n"
                           "-ABCD\n"
                           "\\ No newline at end of file\n"
                           "+ABCD\n");

    QTest::newRow("EOL to last line added") << chunk
                            << fileName
                            << fileName
                            << true
                            << patchText;

    ///////////

    // chunk the same here
    patchText = header + _("@@ -1,1 +1,2 @@\n"
                           " ABCD\n"
                           "+\n");

    QTest::newRow("Last empty line added") << chunk
                            << fileName
                            << fileName
                            << false
                            << patchText;

    ///////////

    rows.clear();
    rows << RowData(_("ABCD"), _("EFGH"));
    chunk.rows = rows;
    patchText = header + _("@@ -1,1 +1,1 @@\n"
                           "-ABCD\n"
                           "+EFGH\n");

    QTest::newRow("Last line with a newline modified") << chunk
                            << fileName
                            << fileName
                            << false
                            << patchText;

    ///////////

    rows.clear();
    rows << RowData(_("ABCD"), _("EFGH"));
    rows << RowData(_(""));
    chunk.rows = rows;
    patchText = header + _("@@ -1,2 +1,2 @@\n"
                           "-ABCD\n"
                           "+EFGH\n"
                           " \n");
    QTest::newRow("Not a last line with a newline modified") << chunk
                            << fileName
                            << fileName
                            << false
                            << patchText;

    ///////////

    rows.clear();
    rows << RowData(_("ABCD"), _("EFGH"));
    chunk.rows = rows;
    patchText = header + _("@@ -1,1 +1,1 @@\n"
                           "-ABCD\n"
                           "\\ No newline at end of file\n"
                           "+EFGH\n"
                           "\\ No newline at end of file\n");

    QTest::newRow("Last line without a newline modified") << chunk
                            << fileName
                            << fileName
                            << true
                            << patchText;

    ///////////

    // chunk the same here
    patchText = header + _("@@ -1,1 +1,1 @@\n"
                           "-ABCD\n"
                           "+EFGH\n");
    QTest::newRow("Not a last line without a newline modified") << chunk
                            << fileName
                            << fileName
                            << false
                            << patchText;

    ///////////

    rows.clear();
    rows << RowData(_("ABCD"), _("EFGH"));
    rows << RowData(_("IJKL"));
    chunk.rows = rows;
    patchText = header + _("@@ -1,2 +1,2 @@\n"
                           "-ABCD\n"
                           "+EFGH\n"
                           " IJKL\n"
                           "\\ No newline at end of file\n");

    QTest::newRow("Last but one line modified, last line without a newline")
            << chunk
            << fileName
            << fileName
            << true
            << patchText;

    ///////////

    // chunk the same here
    patchText = header + _("@@ -1,2 +1,2 @@\n"
                           "-ABCD\n"
                           "+EFGH\n"
                           " IJKL\n");

    QTest::newRow("Last but one line modified, last line with a newline")
            << chunk
            << fileName
            << fileName
            << false
            << patchText;

    ///////////

    rows.clear();
    rows << RowData(_("ABCD"));
    rows << RowData(TextLineData::Separator, _(""));
    rows << RowData(_(""), _("EFGH"));
    chunk.rows = rows;
    patchText = header + _("@@ -1,1 +1,3 @@\n"
                           " ABCD\n"
                           "+\n"
                           "+EFGH\n"
                           "\\ No newline at end of file\n");

    QTest::newRow("Blank line followed by No newline")
            << chunk
            << fileName
            << fileName
            << true
            << patchText;
}

void DiffEditor::Internal::DiffEditorPlugin::testMakePatch()
{
    QFETCH(ChunkData, sourceChunk);
    QFETCH(QString, leftFileName);
    QFETCH(QString, rightFileName);
    QFETCH(bool, lastChunk);
    QFETCH(QString, patchText);

    QString result = DiffUtils::makePatch(sourceChunk, leftFileName, rightFileName, lastChunk);

    QCOMPARE(result, patchText);

    bool ok;
    QList<FileData> resultList = DiffUtils::readPatch(result, &ok);

    QVERIFY(ok);
    QCOMPARE(resultList.count(), 1);
    for (int i = 0; i < resultList.count(); i++) {
        const FileData &resultFileData = resultList.at(i);
        QCOMPARE(resultFileData.leftFileInfo.fileName, leftFileName);
        QCOMPARE(resultFileData.rightFileInfo.fileName, rightFileName);
        QCOMPARE(resultFileData.chunks.count(), 1);
        for (int j = 0; j < resultFileData.chunks.count(); j++) {
            const ChunkData &resultChunkData = resultFileData.chunks.at(j);
            QCOMPARE(resultChunkData.leftStartingLineNumber, sourceChunk.leftStartingLineNumber);
            QCOMPARE(resultChunkData.rightStartingLineNumber, sourceChunk.rightStartingLineNumber);
            QCOMPARE(resultChunkData.contextChunk, sourceChunk.contextChunk);
            QCOMPARE(resultChunkData.rows.count(), sourceChunk.rows.count());
            for (int k = 0; k < sourceChunk.rows.count(); k++) {
                const RowData &sourceRowData = sourceChunk.rows.at(k);
                const RowData &resultRowData = resultChunkData.rows.at(k);
                QCOMPARE(resultRowData.equal, sourceRowData.equal);
                QCOMPARE(resultRowData.leftLine.text, sourceRowData.leftLine.text);
                QCOMPARE(resultRowData.leftLine.textLineType, sourceRowData.leftLine.textLineType);
                QCOMPARE(resultRowData.rightLine.text, sourceRowData.rightLine.text);
                QCOMPARE(resultRowData.rightLine.textLineType, sourceRowData.rightLine.textLineType);
            }
        }
    }
}

void DiffEditor::Internal::DiffEditorPlugin::testReadPatch_data()
{
    QTest::addColumn<QString>("sourcePatch");
    QTest::addColumn<QList<FileData> >("fileDataList");

    QString patch = _("diff --git a/src/plugins/diffeditor/diffeditor.cpp b/src/plugins/diffeditor/diffeditor.cpp\n"
                      "index eab9e9b..082c135 100644\n"
                      "--- a/src/plugins/diffeditor/diffeditor.cpp\n"
                      "+++ b/src/plugins/diffeditor/diffeditor.cpp\n"
                      "@@ -187,9 +187,6 @@ void DiffEditor::ctor()\n"
                      "     m_controller = m_document->controller();\n"
                      "     m_guiController = new DiffEditorGuiController(m_controller, this);\n"
                      " \n"
                      "-//    m_sideBySideEditor->setDiffEditorGuiController(m_guiController);\n"
                      "-//    m_unifiedEditor->setDiffEditorGuiController(m_guiController);\n"
                      "-\n"
                      "     connect(m_controller, SIGNAL(cleared(QString)),\n"
                      "             this, SLOT(slotCleared(QString)));\n"
                      "     connect(m_controller, SIGNAL(diffContentsChanged(QList<DiffEditorController::DiffFilesContents>,QString)),\n"
                      "diff --git a/src/plugins/diffeditor/diffutils.cpp b/src/plugins/diffeditor/diffutils.cpp\n"
                      "index 2f641c9..f8ff795 100644\n"
                      "--- a/src/plugins/diffeditor/diffutils.cpp\n"
                      "+++ b/src/plugins/diffeditor/diffutils.cpp\n"
                      "@@ -464,5 +464,12 @@ QString DiffUtils::makePatch(const ChunkData &chunkData,\n"
                      "     return diffText;\n"
                      " }\n"
                      " \n"
                      "+FileData DiffUtils::makeFileData(const QString &patch)\n"
                      "+{\n"
                      "+    FileData fileData;\n"
                      "+\n"
                      "+    return fileData;\n"
                      "+}\n"
                      "+\n"
                      " } // namespace Internal\n"
                      " } // namespace DiffEditor\n"
                      "diff --git a/new b/new\n"
                      "new file mode 100644\n"
                      "index 0000000..257cc56\n"
                      "--- /dev/null\n"
                      "+++ b/new\n"
                      "@@ -0,0 +1 @@\n"
                      "+foo\n"
                      "diff --git a/deleted b/deleted\n"
                      "deleted file mode 100644\n"
                      "index 257cc56..0000000\n"
                      "--- a/deleted\n"
                      "+++ /dev/null\n"
                      "@@ -1 +0,0 @@\n"
                      "-foo\n"
                      "diff --git a/empty b/empty\n"
                      "new file mode 100644\n"
                      "index 0000000..e69de29\n"
                      "diff --git a/empty b/empty\n"
                      "deleted file mode 100644\n"
                      "index e69de29..0000000\n"
                      "diff --git a/file a.txt b/file b.txt\n"
                      "similarity index 99%\n"
                      "copy from file a.txt\n"
                      "copy to file b.txt\n"
                      "index 1234567..9876543\n"
                      "--- a/file a.txt\n"
                      "+++ b/file b.txt\n"
                      "@@ -20,3 +20,3 @@\n"
                      " A\n"
                      "-B\n"
                      "+C\n"
                      " D\n"
                      "diff --git a/file a.txt b/file b.txt\n"
                      "similarity index 99%\n"
                      "rename from file a.txt\n"
                      "rename to file b.txt\n"
                      "diff --git a/file.txt b/file.txt\n"
                      "old mode 100644\n"
                      "new mode 100755\n"
                      "index 1234567..9876543\n"
                      "--- a/file.txt\n"
                      "+++ b/file.txt\n"
                      "@@ -20,3 +20,3 @@\n"
                      " A\n"
                      "-B\n"
                      "+C\n"
                      " D\n"
                      );

    FileData fileData1;
    fileData1.leftFileInfo = DiffFileInfo(_("src/plugins/diffeditor/diffeditor.cpp"), _("eab9e9b"));
    fileData1.rightFileInfo = DiffFileInfo(_("src/plugins/diffeditor/diffeditor.cpp"), _("082c135"));
    ChunkData chunkData1;
    chunkData1.leftStartingLineNumber = 186;
    chunkData1.rightStartingLineNumber = 186;
    QList<RowData> rows1;
    rows1 << RowData(_("    m_controller = m_document->controller();"));
    rows1 << RowData(_("    m_guiController = new DiffEditorGuiController(m_controller, this);"));
    rows1 << RowData(_(""));
    rows1 << RowData(_("//    m_sideBySideEditor->setDiffEditorGuiController(m_guiController);"), TextLineData::Separator);
    rows1 << RowData(_("//    m_unifiedEditor->setDiffEditorGuiController(m_guiController);"), TextLineData::Separator);
    rows1 << RowData(_(""), TextLineData::Separator);
    rows1 << RowData(_("    connect(m_controller, SIGNAL(cleared(QString)),"));
    rows1 << RowData(_("            this, SLOT(slotCleared(QString)));"));
    rows1 << RowData(_("    connect(m_controller, SIGNAL(diffContentsChanged(QList<DiffEditorController::DiffFilesContents>,QString)),"));
    chunkData1.rows = rows1;
    fileData1.chunks << chunkData1;

    FileData fileData2;
    fileData2.leftFileInfo = DiffFileInfo(_("src/plugins/diffeditor/diffutils.cpp"), _("2f641c9"));
    fileData2.rightFileInfo = DiffFileInfo(_("src/plugins/diffeditor/diffutils.cpp"), _("f8ff795"));
    ChunkData chunkData2;
    chunkData2.leftStartingLineNumber = 463;
    chunkData2.rightStartingLineNumber = 463;
    QList<RowData> rows2;
    rows2 << RowData(_("    return diffText;"));
    rows2 << RowData(_("}"));
    rows2 << RowData(_(""));
    rows2 << RowData(TextLineData::Separator, _("FileData DiffUtils::makeFileData(const QString &patch)"));
    rows2 << RowData(TextLineData::Separator, _("{"));
    rows2 << RowData(TextLineData::Separator, _("    FileData fileData;"));
    rows2 << RowData(TextLineData::Separator, _(""));
    rows2 << RowData(TextLineData::Separator, _("    return fileData;"));
    rows2 << RowData(TextLineData::Separator, _("}"));
    rows2 << RowData(TextLineData::Separator, _(""));
    rows2 << RowData(_("} // namespace Internal"));
    rows2 << RowData(_("} // namespace DiffEditor"));
    chunkData2.rows = rows2;
    fileData2.chunks << chunkData2;

    FileData fileData3;
    fileData3.leftFileInfo = DiffFileInfo(_("new"), _("0000000"));
    fileData3.rightFileInfo = DiffFileInfo(_("new"), _("257cc56"));
    fileData3.fileOperation = FileData::NewFile;
    ChunkData chunkData3;
    chunkData3.leftStartingLineNumber = -1;
    chunkData3.rightStartingLineNumber = 0;
    QList<RowData> rows3;
    rows3 << RowData(TextLineData::Separator, _("foo"));
    TextLineData textLineData3(TextLineData::TextLine);
    rows3 << RowData(TextLineData::Separator, textLineData3);
    chunkData3.rows = rows3;
    fileData3.chunks << chunkData3;

    FileData fileData4;
    fileData4.leftFileInfo = DiffFileInfo(_("deleted"), _("257cc56"));
    fileData4.rightFileInfo = DiffFileInfo(_("deleted"), _("0000000"));
    fileData4.fileOperation = FileData::DeleteFile;
    ChunkData chunkData4;
    chunkData4.leftStartingLineNumber = 0;
    chunkData4.rightStartingLineNumber = -1;
    QList<RowData> rows4;
    rows4 << RowData(_("foo"), TextLineData::Separator);
    TextLineData textLineData4(TextLineData::TextLine);
    rows4 << RowData(textLineData4, TextLineData::Separator);
    chunkData4.rows = rows4;
    fileData4.chunks << chunkData4;

    FileData fileData5;
    fileData5.leftFileInfo = DiffFileInfo(_("empty"), _("0000000"));
    fileData5.rightFileInfo = DiffFileInfo(_("empty"), _("e69de29"));
    fileData5.fileOperation = FileData::NewFile;

    FileData fileData6;
    fileData6.leftFileInfo = DiffFileInfo(_("empty"), _("e69de29"));
    fileData6.rightFileInfo = DiffFileInfo(_("empty"), _("0000000"));
    fileData6.fileOperation = FileData::DeleteFile;

    FileData fileData7;
    fileData7.leftFileInfo = DiffFileInfo(_("file a.txt"), _("1234567"));
    fileData7.rightFileInfo = DiffFileInfo(_("file b.txt"), _("9876543"));
    fileData7.fileOperation = FileData::CopyFile;
    ChunkData chunkData7;
    chunkData7.leftStartingLineNumber = 19;
    chunkData7.rightStartingLineNumber = 19;
    QList<RowData> rows7;
    rows7 << RowData(_("A"));
    rows7 << RowData(_("B"), _("C"));
    rows7 << RowData(_("D"));
    chunkData7.rows = rows7;
    fileData7.chunks << chunkData7;

    FileData fileData8;
    fileData8.leftFileInfo = DiffFileInfo(_("file a.txt"));
    fileData8.rightFileInfo = DiffFileInfo(_("file b.txt"));
    fileData8.fileOperation = FileData::RenameFile;

    FileData fileData9;
    fileData9.leftFileInfo = DiffFileInfo(_("file.txt"), _("1234567"));
    fileData9.rightFileInfo = DiffFileInfo(_("file.txt"), _("9876543"));
    fileData9.chunks << chunkData7;
    QList<FileData> fileDataList1;
    fileDataList1 << fileData1 << fileData2 << fileData3 << fileData4 << fileData5
                  << fileData6 << fileData7 << fileData8 << fileData9;

    QTest::newRow("Git patch") << patch
                               << fileDataList1;

    //////////////

    patch = _("diff --git a/file foo.txt b/file foo.txt\n"
              "index 1234567..9876543 100644\n"
              "--- a/file foo.txt\n"
              "+++ b/file foo.txt\n"
              "@@ -50,4 +50,5 @@ void DiffEditor::ctor()\n"
              " A\n"
              " B\n"
              " C\n"
              "+\n");

    fileData1.leftFileInfo = DiffFileInfo(_("file foo.txt"), _("1234567"));
    fileData1.rightFileInfo = DiffFileInfo(_("file foo.txt"), _("9876543"));
    fileData1.fileOperation = FileData::ChangeFile;
    chunkData1.leftStartingLineNumber = 49;
    chunkData1.rightStartingLineNumber = 49;
    rows1.clear();
    rows1 << RowData(_("A"));
    rows1 << RowData(_("B"));
    rows1 << RowData(_("C"));
    rows1 << RowData(TextLineData::Separator, _(""));
    chunkData1.rows = rows1;
    fileData1.chunks.clear();
    fileData1.chunks << chunkData1;

    QList<FileData> fileDataList2;
    fileDataList2 << fileData1;

    QTest::newRow("Added line") << patch
                                << fileDataList2;

    //////////////

    patch = _("diff --git a/file foo.txt b/file foo.txt\n"
              "index 1234567..9876543 100644\n"
              "--- a/file foo.txt\n"
              "+++ b/file foo.txt\n"
              "@@ -1,1 +1,1 @@\n"
              "-ABCD\n"
              "\\ No newline at end of file\n"
              "+ABCD\n");

    fileData1.leftFileInfo = DiffFileInfo(_("file foo.txt"), _("1234567"));
    fileData1.rightFileInfo = DiffFileInfo(_("file foo.txt"), _("9876543"));
    fileData1.fileOperation = FileData::ChangeFile;
    chunkData1.leftStartingLineNumber = 0;
    chunkData1.rightStartingLineNumber = 0;
    rows1.clear();
    rows1 << RowData(_("ABCD"));
    rows1 << RowData(TextLineData::Separator, _(""));
    chunkData1.rows = rows1;
    fileData1.chunks.clear();
    fileData1.chunks << chunkData1;

    QList<FileData> fileDataList3;
    fileDataList3 << fileData1;

    QTest::newRow("Last newline added to a line without newline") << patch
                                << fileDataList3;

    patch = _("diff --git a/difftest.txt b/difftest.txt\n"
              "index 1234567..9876543 100644\n"
              "--- a/difftest.txt\n"
              "+++ b/difftest.txt\n"
              "@@ -2,5 +2,5 @@ void func()\n"
              " A\n"
              " B\n"
              "-C\n"
              "+Z\n"
              " D\n"
              " \n"
              "@@ -9,2 +9,4 @@ void OtherFunc()\n"
              " \n"
              " D\n"
              "+E\n"
              "+F\n"
              );

    fileData1.leftFileInfo = DiffFileInfo(_("difftest.txt"), _("1234567"));
    fileData1.rightFileInfo = DiffFileInfo(_("difftest.txt"), _("9876543"));
    fileData1.fileOperation = FileData::ChangeFile;
    chunkData1.leftStartingLineNumber = 1;
    chunkData1.rightStartingLineNumber = 1;
    rows1.clear();
    rows1 << RowData(_("A"));
    rows1 << RowData(_("B"));
    rows1 << RowData(_("C"), _("Z"));
    rows1 << RowData(_("D"));
    rows1 << RowData(_(""));
    chunkData1.rows = rows1;

    chunkData2.leftStartingLineNumber = 8;
    chunkData2.rightStartingLineNumber = 8;
    rows2.clear();
    rows2 << RowData(_(""));
    rows2 << RowData(_("D"));
    rows2 << RowData(TextLineData::Separator, _("E"));
    rows2 << RowData(TextLineData::Separator, _("F"));
    chunkData2.rows = rows2;
    fileData1.chunks.clear();
    fileData1.chunks << chunkData1;
    fileData1.chunks << chunkData2;

    QList<FileData> fileDataList4;
    fileDataList4 << fileData1;

    QTest::newRow("2 chunks - first ends with blank line") << patch
                                << fileDataList4;

    //////////////

    patch = _("diff --git a/file foo.txt b/file foo.txt\n"
              "index 1234567..9876543 100644\n"
              "--- a/file foo.txt\n"
              "+++ b/file foo.txt\n"
              "@@ -1,1 +1,3 @@ void DiffEditor::ctor()\n"
              " ABCD\n"
              "+\n"
              "+EFGH\n"
              "\\ No newline at end of file\n");

    fileData1.leftFileInfo = DiffFileInfo(_("file foo.txt"), _("1234567"));
    fileData1.rightFileInfo = DiffFileInfo(_("file foo.txt"), _("9876543"));
    fileData1.fileOperation = FileData::ChangeFile;
    chunkData1.leftStartingLineNumber = 0;
    chunkData1.rightStartingLineNumber = 0;
    rows1.clear();
    rows1 << RowData(_("ABCD"));
    rows1 << RowData(TextLineData::Separator, _(""));
    rows1 << RowData(_(""), _("EFGH"));
    chunkData1.rows = rows1;
    fileData1.chunks.clear();
    fileData1.chunks << chunkData1;

    QList<FileData> fileDataList5;
    fileDataList5 << fileData1;

    QTest::newRow("Blank line followed by No newline") << patch
                                << fileDataList5;

    //////////////

    // Based on 953cdb97
    patch = _("diff --git a/src/plugins/texteditor/basetextdocument.h b/src/plugins/texteditor/textdocument.h\n"
              "similarity index 100%\n"
              "rename from src/plugins/texteditor/basetextdocument.h\n"
              "rename to src/plugins/texteditor/textdocument.h\n"
              "diff --git a/src/plugins/texteditor/basetextdocumentlayout.cpp b/src/plugins/texteditor/textdocumentlayout.cpp\n"
              "similarity index 79%\n"
              "rename from src/plugins/texteditor/basetextdocumentlayout.cpp\n"
              "rename to src/plugins/texteditor/textdocumentlayout.cpp\n"
              "index 0121933..01cc3a0 100644\n"
              "--- a/src/plugins/texteditor/basetextdocumentlayout.cpp\n"
              "+++ b/src/plugins/texteditor/textdocumentlayout.cpp\n"
              "@@ -2,5 +2,5 @@ void func()\n"
              " A\n"
              " B\n"
              "-C\n"
              "+Z\n"
              " D\n"
              " \n"
              );

    fileData1 = FileData();
    fileData1.leftFileInfo = DiffFileInfo(_("src/plugins/texteditor/basetextdocument.h"));
    fileData1.rightFileInfo = DiffFileInfo(_("src/plugins/texteditor/textdocument.h"));
    fileData1.fileOperation = FileData::RenameFile;
    fileData2 = FileData();
    fileData2.leftFileInfo = DiffFileInfo(_("src/plugins/texteditor/basetextdocumentlayout.cpp"), _("0121933"));
    fileData2.rightFileInfo = DiffFileInfo(_("src/plugins/texteditor/textdocumentlayout.cpp"), _("01cc3a0"));
    fileData2.fileOperation = FileData::RenameFile;
    chunkData2.leftStartingLineNumber = 1;
    chunkData2.rightStartingLineNumber = 1;
    rows2.clear();
    rows2 << RowData(_("A"));
    rows2 << RowData(_("B"));
    rows2 << RowData(_("C"), _("Z"));
    rows2 << RowData(_("D"));
    rows2 << RowData(_(""));
    chunkData2.rows = rows2;
    fileData2.chunks.clear();
    fileData2.chunks << chunkData2;

    QList<FileData> fileDataList6;
    fileDataList6 << fileData1 << fileData2;

    QTest::newRow("Multiple renames") << patch
                                      << fileDataList6;

    //////////////

    // Dirty submodule
    patch = _("diff --git a/src/shared/qbs b/src/shared/qbs\n"
              "--- a/src/shared/qbs\n"
              "+++ b/src/shared/qbs\n"
              "@@ -1 +1 @@\n"
              "-Subproject commit eda76354077a427d692fee05479910de31040d3f\n"
              "+Subproject commit eda76354077a427d692fee05479910de31040d3f-dirty\n"
              );
    fileData1 = FileData();
    fileData1.leftFileInfo = DiffFileInfo(_("src/shared/qbs"));
    fileData1.rightFileInfo = DiffFileInfo(_("src/shared/qbs"));
    chunkData1.leftStartingLineNumber = 0;
    chunkData1.rightStartingLineNumber = 0;
    rows1.clear();
    rows1 << RowData(_("Subproject commit eda76354077a427d692fee05479910de31040d3f"),
                     _("Subproject commit eda76354077a427d692fee05479910de31040d3f-dirty"));
    chunkData1.rows = rows1;
    fileData1.chunks.clear();
    fileData1.chunks <<  chunkData1;

    QList<FileData> fileDataList7;
    fileDataList7 << fileData1;
    QTest::newRow("Dirty submodule") << patch
                                     << fileDataList7;

    //////////////

    // Subversion New
    patch = _("Index: src/plugins/subversion/subversioneditor.cpp\n"
              "===================================================================\n"
              "--- src/plugins/subversion/subversioneditor.cpp\t(revision 0)\n"
              "+++ src/plugins/subversion/subversioneditor.cpp\t(revision 0)\n"
              "@@ -0,0 +125 @@\n\n");
    fileData1 = FileData();
    fileData1.leftFileInfo = DiffFileInfo(_("src/plugins/subversion/subversioneditor.cpp"));
    fileData1.rightFileInfo = DiffFileInfo(_("src/plugins/subversion/subversioneditor.cpp"));
    chunkData1 = ChunkData();
    chunkData1.leftStartingLineNumber = -1;
    chunkData1.rightStartingLineNumber = 124;
    fileData1.chunks << chunkData1;
    QList<FileData> fileDataList8;
    fileDataList8 << fileData1;
    QTest::newRow("Subversion New") << patch
                                    << fileDataList8;

    //////////////

    // Subversion Deleted
    patch = _("Index: src/plugins/subversion/subversioneditor.cpp\n"
              "===================================================================\n"
              "--- src/plugins/subversion/subversioneditor.cpp\t(revision 42)\n"
              "+++ src/plugins/subversion/subversioneditor.cpp\t(working copy)\n"
              "@@ -1,125 +0,0 @@\n\n");
    fileData1 = FileData();
    fileData1.leftFileInfo = DiffFileInfo(_("src/plugins/subversion/subversioneditor.cpp"));
    fileData1.rightFileInfo = DiffFileInfo(_("src/plugins/subversion/subversioneditor.cpp"));
    chunkData1 = ChunkData();
    chunkData1.leftStartingLineNumber = 0;
    chunkData1.rightStartingLineNumber = -1;
    fileData1.chunks << chunkData1;
    QList<FileData> fileDataList9;
    fileDataList9 << fileData1;
    QTest::newRow("Subversion Deleted") << patch
                                        << fileDataList9;

    //////////////

    // Subversion Normal
    patch = _("Index: src/plugins/subversion/subversioneditor.cpp\n"
              "===================================================================\n"
              "--- src/plugins/subversion/subversioneditor.cpp\t(revision 42)\n"
              "+++ src/plugins/subversion/subversioneditor.cpp\t(working copy)\n"
              "@@ -120,7 +120,7 @@\n\n");
    fileData1 = FileData();
    fileData1.leftFileInfo = DiffFileInfo(_("src/plugins/subversion/subversioneditor.cpp"));
    fileData1.rightFileInfo = DiffFileInfo(_("src/plugins/subversion/subversioneditor.cpp"));
    chunkData1 = ChunkData();
    chunkData1.leftStartingLineNumber = 119;
    chunkData1.rightStartingLineNumber = 119;
    fileData1.chunks << chunkData1;
    QList<FileData> fileDataList10;
    fileDataList10 << fileData1;
    QTest::newRow("Subversion Normal") << patch
                                       << fileDataList10;
}

void DiffEditor::Internal::DiffEditorPlugin::testReadPatch()
{
    QFETCH(QString, sourcePatch);
    QFETCH(QList<FileData>, fileDataList);

    bool ok;
    QList<FileData> result = DiffUtils::readPatch(sourcePatch, &ok);

    QVERIFY(ok);
    QCOMPARE(fileDataList.count(), result.count());
    for (int i = 0; i < fileDataList.count(); i++) {
        const FileData &origFileData = fileDataList.at(i);
        const FileData &resultFileData = result.at(i);
        QCOMPARE(resultFileData.leftFileInfo.fileName, origFileData.leftFileInfo.fileName);
        QCOMPARE(resultFileData.leftFileInfo.typeInfo, origFileData.leftFileInfo.typeInfo);
        QCOMPARE(resultFileData.rightFileInfo.fileName, origFileData.rightFileInfo.fileName);
        QCOMPARE(resultFileData.rightFileInfo.typeInfo, origFileData.rightFileInfo.typeInfo);
        QCOMPARE(resultFileData.chunks.count(), origFileData.chunks.count());
        QCOMPARE(resultFileData.fileOperation, origFileData.fileOperation);
        for (int j = 0; j < origFileData.chunks.count(); j++) {
            const ChunkData &origChunkData = origFileData.chunks.at(j);
            const ChunkData &resultChunkData = resultFileData.chunks.at(j);
            QCOMPARE(resultChunkData.leftStartingLineNumber, origChunkData.leftStartingLineNumber);
            QCOMPARE(resultChunkData.rightStartingLineNumber, origChunkData.rightStartingLineNumber);
            QCOMPARE(resultChunkData.contextChunk, origChunkData.contextChunk);
            QCOMPARE(resultChunkData.rows.count(), origChunkData.rows.count());
            for (int k = 0; k < origChunkData.rows.count(); k++) {
                const RowData &origRowData = origChunkData.rows.at(k);
                const RowData &resultRowData = resultChunkData.rows.at(k);
                QCOMPARE(resultRowData.equal, origRowData.equal);
                QCOMPARE(resultRowData.leftLine.text, origRowData.leftLine.text);
                QCOMPARE(resultRowData.leftLine.textLineType, origRowData.leftLine.textLineType);
                QCOMPARE(resultRowData.rightLine.text, origRowData.rightLine.text);
                QCOMPARE(resultRowData.rightLine.textLineType, origRowData.rightLine.textLineType);
            }
        }
    }
}

#endif // WITH_TESTS

#include "diffeditorplugin.moc"
