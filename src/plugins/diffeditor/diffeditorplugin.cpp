/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "diffeditorplugin.h"
#include "diffeditor.h"
#include "diffeditorconstants.h"
#include "diffeditordocument.h"
#include "diffeditorfactory.h"
#include "diffeditormanager.h"
#include "diffeditorreloader.h"
#include "differ.h"

#include <QFileDialog>
#include <QTextCodec>
#include <QtPlugin>

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>

namespace DiffEditor {
namespace Internal {

class SimpleDiffEditorReloader : public DiffEditorReloader
{
    Q_OBJECT
public:
    SimpleDiffEditorReloader(QObject *parent,
                             const QString &leftFileName,
                             const QString &rightFileName);

protected:
    void reload();

private:
    QString getFileContents(const QString &fileName) const;

    QString m_leftFileName;
    QString m_rightFileName;
};

SimpleDiffEditorReloader::SimpleDiffEditorReloader(QObject *parent,
                                                   const QString &leftFileName,
                                                   const QString &rightFileName)
    : DiffEditorReloader(parent),
      m_leftFileName(leftFileName),
      m_rightFileName(rightFileName)
{
}

void SimpleDiffEditorReloader::reload()
{
    const QString leftText = getFileContents(m_leftFileName);
    const QString rightText = getFileContents(m_rightFileName);

    Differ differ;
    QList<Diff> diffList = differ.cleanupSemantics(
                differ.diff(leftText, rightText));

    QList<Diff> leftDiffList;
    QList<Diff> rightDiffList;
    Differ::splitDiffList(diffList, &leftDiffList, &rightDiffList);
    QList<Diff> outputLeftDiffList;
    QList<Diff> outputRightDiffList;

    if (diffEditorController()->isIgnoreWhitespace()) {
        const QList<Diff> leftIntermediate =
                Differ::moveWhitespaceIntoEqualities(leftDiffList);
        const QList<Diff> rightIntermediate =
                Differ::moveWhitespaceIntoEqualities(rightDiffList);
        Differ::ignoreWhitespaceBetweenEqualities(leftIntermediate,
                                                  rightIntermediate,
                                                  &outputLeftDiffList,
                                                  &outputRightDiffList);
    } else {
        outputLeftDiffList = leftDiffList;
        outputRightDiffList = rightDiffList;
    }

    const ChunkData chunkData = DiffUtils::calculateOriginalData(
                outputLeftDiffList, outputRightDiffList);
    FileData fileData = DiffUtils::calculateContextData(
                chunkData, diffEditorController()->contextLinesNumber(), 0);
    fileData.leftFileInfo.fileName = m_leftFileName;
    fileData.rightFileInfo.fileName = m_rightFileName;

    QList<FileData> fileDataList;
    fileDataList << fileData;

    diffEditorController()->setDiffFiles(fileDataList);

    reloadFinished();
}

QString SimpleDiffEditorReloader::getFileContents(const QString &fileName) const
{
    QFile file(fileName);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text))
        return Core::EditorManager::defaultTextCodec()->toUnicode(file.readAll());
    return QString();
}

/////////////////

DiffEditorPlugin::DiffEditorPlugin()
{
}

DiffEditorPlugin::~DiffEditorPlugin()
{
}

bool DiffEditorPlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorMessage)

    //register actions
    Core::ActionContainer *toolsContainer =
        Core::ActionManager::actionContainer(Core::Constants::M_TOOLS);
    toolsContainer->insertGroup(Core::Constants::G_TOOLS_OPTIONS,
                                Constants::G_TOOLS_DIFF);

    Core::Context globalcontext(Core::Constants::C_GLOBAL);

    QAction *diffAction = new QAction(tr("Diff..."), this);
    Core::Command *diffCommand = Core::ActionManager::registerAction(diffAction,
                             "DiffEditor.Diff", globalcontext);
    connect(diffAction, SIGNAL(triggered()), this, SLOT(diff()));
    toolsContainer->addAction(diffCommand, Constants::G_TOOLS_DIFF);

    addAutoReleasedObject(new DiffEditorFactory(this));

    new DiffEditorManager(this);

    return true;
}

void DiffEditorPlugin::extensionsInitialized()
{
}

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


    const QString documentId = QLatin1String("Diff ") + fileName1
            + QLatin1String(", ") + fileName2;
    DiffEditorDocument *document = DiffEditorManager::find(documentId);
    if (!document) {
        QString title = tr("Diff \"%1\", \"%2\"").arg(fileName1).arg(fileName2);
        document = DiffEditorManager::findOrCreate(documentId, title);
        if (!document)
            return;

        DiffEditorController *controller = document->controller();
        SimpleDiffEditorReloader *reloader =
                new SimpleDiffEditorReloader(controller, fileName1, fileName2);
        reloader->setDiffEditorController(controller);
    }

    Core::EditorManager::activateEditorForDocument(document);

    document->controller()->requestReload();
}

} // namespace Internal
} // namespace DiffEditor

#ifdef WITH_TESTS

#include <QTest>

#include "diffutils.h"

Q_DECLARE_METATYPE(DiffEditor::ChunkData)
Q_DECLARE_METATYPE(QList<DiffEditor::FileData>)

void DiffEditor::Internal::DiffEditorPlugin::testMakePatch_data()
{
    QTest::addColumn<ChunkData>("sourceChunk");
    QTest::addColumn<QString>("leftFileName");
    QTest::addColumn<QString>("rightFileName");
    QTest::addColumn<bool>("lastChunk");
    QTest::addColumn<QString>("patchText");

    const QString fileName = QLatin1String("a.txt");
    const QString header = QLatin1String("--- ") + fileName
            + QLatin1String("\n+++ ") + fileName + QLatin1String("\n");

    QList<RowData> rows;
    rows << RowData(TextLineData(QLatin1String("ABCD")),
                    TextLineData(TextLineData::Separator));
    rows << RowData(TextLineData(QLatin1String("EFGH")));
    rows << RowData(TextLineData(QLatin1String("")));
    ChunkData chunk;
    chunk.rows = rows;
    QString patchText = header + QLatin1String("@@ -1,2 +1,1 @@\n"
                                               "-ABCD\n"
                                               " EFGH\n");
    QTest::newRow("Simple") << chunk
                            << fileName
                            << fileName
                            << true
                            << patchText;

    ///////////

    rows.clear();
    rows << RowData(TextLineData(QLatin1String("ABCD")),
                    TextLineData(QLatin1String("ABCD")));
    rows << RowData(TextLineData(QLatin1String("")),
                    TextLineData(TextLineData::Separator));
    chunk.rows = rows;
    patchText = header + QLatin1String("@@ -1,1 +1,1 @@\n"
                                       "-ABCD\n"
                                       "+ABCD\n"
                                       "\\ No newline at end of file\n");

    QTest::newRow("Last newline removed") << chunk
                            << fileName
                            << fileName
                            << true
                            << patchText;

    ///////////

    // chunk the same here
    patchText = header + QLatin1String("@@ -1,2 +1,1 @@\n"
                                       "-ABCD\n"
                                       "-\n"
                                       "+ABCD\n");

    QTest::newRow("Not a last newline removed") << chunk
                            << fileName
                            << fileName
                            << false
                            << patchText;

    ///////////

    rows.clear();
    rows << RowData(TextLineData(QLatin1String("ABCD")),
                    TextLineData(QLatin1String("ABCD")));
    rows << RowData(TextLineData(TextLineData::Separator),
                    TextLineData(QLatin1String("")));
    chunk.rows = rows;
    patchText = header + QLatin1String("@@ -1,1 +1,1 @@\n"
                                       "-ABCD\n"
                                       "\\ No newline at end of file\n"
                                       "+ABCD\n");

    QTest::newRow("Last newline added") << chunk
                            << fileName
                            << fileName
                            << true
                            << patchText;

    ///////////

    // chunk the same here
    patchText = header + QLatin1String("@@ -1,1 +1,2 @@\n"
                                       "-ABCD\n"
                                       "+ABCD\n"
                                       "+\n");

    QTest::newRow("Not a last newline added") << chunk
                            << fileName
                            << fileName
                            << false
                            << patchText;

    ///////////

    rows.clear();
    rows << RowData(TextLineData(QLatin1String("ABCD")),
                    TextLineData(QLatin1String("EFGH")));
    rows << RowData(TextLineData(QLatin1String("")));
    chunk.rows = rows;
    patchText = header + QLatin1String("@@ -1,1 +1,1 @@\n"
                                       "-ABCD\n"
                                       "+EFGH\n");

    QTest::newRow("Last line with a newline modified") << chunk
                            << fileName
                            << fileName
                            << true
                            << patchText;

    ///////////

    // chunk the same here
    patchText = header + QLatin1String("@@ -1,2 +1,2 @@\n"
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
    rows << RowData(TextLineData(QLatin1String("ABCD")),
                    TextLineData(QLatin1String("EFGH")));
    chunk.rows = rows;
    patchText = header + QLatin1String("@@ -1,1 +1,1 @@\n"
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
    patchText = header + QLatin1String("@@ -1,1 +1,1 @@\n"
                                       "-ABCD\n"
                                       "+EFGH\n");
    QTest::newRow("Not a last line without a newline modified") << chunk
                            << fileName
                            << fileName
                            << false
                            << patchText;

    ///////////

    rows.clear();
    rows << RowData(TextLineData(QLatin1String("ABCD")),
                    TextLineData(QLatin1String("EFGH")));
    rows << RowData(TextLineData(QLatin1String("IJKL")));
    chunk.rows = rows;
    patchText = header + QLatin1String("@@ -1,2 +1,2 @@\n"
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
    patchText = header + QLatin1String("@@ -1,2 +1,2 @@\n"
                                       "-ABCD\n"
                                       "+EFGH\n"
                                       " IJKL\n");

    QTest::newRow("Last but one line modified, last line with a newline")
            << chunk
            << fileName
            << fileName
            << false
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

    QCOMPARE(patchText, result);
}

void DiffEditor::Internal::DiffEditorPlugin::testReadPatch_data()
{
    QTest::addColumn<QString>("sourcePatch");
    QTest::addColumn<QList<FileData> >("fileDataList");

    QString patch = QLatin1String("diff --git a/src/plugins/diffeditor/diffeditor.cpp b/src/plugins/diffeditor/diffeditor.cpp\n"
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
                                  );

    FileData fileData1;
    fileData1.leftFileInfo = DiffFileInfo(QLatin1String("src/plugins/diffeditor/diffeditor.cpp"),
                                          QLatin1String("eab9e9b"));
    fileData1.rightFileInfo = DiffFileInfo(QLatin1String("src/plugins/diffeditor/diffeditor.cpp"),
                                           QLatin1String("082c135"));
    ChunkData chunkData1;
    chunkData1.leftStartingLineNumber = 186;
    chunkData1.rightStartingLineNumber = 186;
    QList<RowData> rows1;
    rows1.append(RowData(TextLineData(QLatin1String("    m_controller = m_document->controller();"))));
    rows1.append(RowData(TextLineData(QLatin1String("    m_guiController = new DiffEditorGuiController(m_controller, this);"))));
    rows1.append(RowData(TextLineData(QLatin1String(""))));
    rows1.append(RowData(TextLineData(QLatin1String("//    m_sideBySideEditor->setDiffEditorGuiController(m_guiController);")),
                         TextLineData(TextLineData::Separator)));
    rows1.append(RowData(TextLineData(QLatin1String("//    m_unifiedEditor->setDiffEditorGuiController(m_guiController);")),
                         TextLineData(TextLineData::Separator)));
    rows1.append(RowData(TextLineData(QLatin1String("")),
                         TextLineData(TextLineData::Separator)));
    rows1.append(RowData(TextLineData(QLatin1String("    connect(m_controller, SIGNAL(cleared(QString)),"))));
    rows1.append(RowData(TextLineData(QLatin1String("            this, SLOT(slotCleared(QString)));"))));
    rows1.append(RowData(TextLineData(QLatin1String("    connect(m_controller, SIGNAL(diffContentsChanged(QList<DiffEditorController::DiffFilesContents>,QString)),"))));
    chunkData1.rows = rows1;
    fileData1.chunks.append(chunkData1);

    FileData fileData2;
    fileData2.leftFileInfo = DiffFileInfo(QLatin1String("src/plugins/diffeditor/diffutils.cpp"),
                                          QLatin1String("2f641c9"));
    fileData2.rightFileInfo = DiffFileInfo(QLatin1String("src/plugins/diffeditor/diffutils.cpp"),
                                           QLatin1String("f8ff795"));
    ChunkData chunkData2;
    chunkData2.leftStartingLineNumber = 463;
    chunkData2.rightStartingLineNumber = 463;
    QList<RowData> rows2;
    rows2.append(RowData(TextLineData(QLatin1String("    return diffText;"))));
    rows2.append(RowData(TextLineData(QLatin1String("}"))));
    rows2.append(RowData(TextLineData(QLatin1String(""))));
    rows2.append(RowData(TextLineData(TextLineData::Separator),
                         TextLineData(QLatin1String("FileData DiffUtils::makeFileData(const QString &patch)"))));
    rows2.append(RowData(TextLineData(TextLineData::Separator),
                         TextLineData(QLatin1String("{"))));
    rows2.append(RowData(TextLineData(TextLineData::Separator),
                         TextLineData(QLatin1String("    FileData fileData;"))));
    rows2.append(RowData(TextLineData(TextLineData::Separator),
                         TextLineData(QLatin1String(""))));
    rows2.append(RowData(TextLineData(TextLineData::Separator),
                         TextLineData(QLatin1String("    return fileData;"))));
    rows2.append(RowData(TextLineData(TextLineData::Separator),
                         TextLineData(QLatin1String("}"))));
    rows2.append(RowData(TextLineData(TextLineData::Separator),
                         TextLineData(QLatin1String(""))));
    rows2.append(RowData(TextLineData(QLatin1String("} // namespace Internal"))));
    rows2.append(RowData(TextLineData(QLatin1String("} // namespace DiffEditor"))));
    chunkData2.rows = rows2;
    fileData2.chunks.append(chunkData2);

    FileData fileData3;
    fileData3.leftFileInfo = DiffFileInfo(QLatin1String("new"), QLatin1String("0000000"));
    fileData3.rightFileInfo = DiffFileInfo(QLatin1String("new"), QLatin1String("257cc56"));
    ChunkData chunkData3;
    chunkData3.leftStartingLineNumber = -1;
    chunkData3.rightStartingLineNumber = 0;
    QList<RowData> rows3;
    rows3.append(RowData(TextLineData::Separator, TextLineData(QLatin1String("foo"))));
    TextLineData textLineData3(TextLineData::TextLine);
    rows3.append(RowData(TextLineData::Separator, textLineData3));
    chunkData3.rows = rows3;
    fileData3.chunks.append(chunkData3);

    FileData fileData4;
    fileData4.leftFileInfo = DiffFileInfo(QLatin1String("deleted"), QLatin1String("257cc56"));
    fileData4.rightFileInfo = DiffFileInfo(QLatin1String("deleted"), QLatin1String("0000000"));
    ChunkData chunkData4;
    chunkData4.leftStartingLineNumber = 0;
    chunkData4.rightStartingLineNumber = -1;
    QList<RowData> rows4;
    rows4.append(RowData(TextLineData(QLatin1String("foo")), TextLineData::Separator));
    TextLineData textLineData4(TextLineData::TextLine);
    rows4.append(RowData(textLineData4, TextLineData::Separator));
    chunkData4.rows = rows4;
    fileData4.chunks.append(chunkData4);

    QList<FileData> fileDataList;
    fileDataList << fileData1 << fileData2 << fileData3 << fileData4;

    QTest::newRow("Git patch") << patch
                               << fileDataList;
}

void DiffEditor::Internal::DiffEditorPlugin::testReadPatch()
{
    QFETCH(QString, sourcePatch);
    QFETCH(QList<FileData>, fileDataList);

    bool ok;
    QList<FileData> result = DiffUtils::readPatch(sourcePatch, false, &ok);

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


Q_EXPORT_PLUGIN(DiffEditor::Internal::DiffEditorPlugin)

#include "diffeditorplugin.moc"
