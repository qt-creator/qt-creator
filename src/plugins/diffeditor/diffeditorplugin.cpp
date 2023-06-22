// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "diffeditorplugin.h"
#include "diffeditorconstants.h"
#include "diffeditorcontroller.h"
#include "diffeditordocument.h"
#include "diffeditorfactory.h"
#include "diffeditortr.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/editormanager/documentmodel.h>
#include <coreplugin/editormanager/editormanager.h>

#include <extensionsystem/pluginmanager.h>

#include <texteditor/textdocument.h>

#include <utils/async.h>
#include <utils/differ.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>

#include <QAction>
#include <QMenu>

using namespace Core;
using namespace TextEditor;
using namespace Utils;

namespace DiffEditor::Internal {

class ReloadInput {
public:
    std::array<QString, SideCount> text{};
    DiffFileInfoArray fileInfo{};
    FileData::FileOperation fileOperation = FileData::ChangeFile;
    bool binaryFiles = false;
};

class DiffFile
{
public:
    DiffFile(bool ignoreWhitespace, int contextLineCount)
        : m_contextLineCount(contextLineCount),
          m_ignoreWhitespace(ignoreWhitespace)
    {}

    void operator()(QPromise<FileData> &promise, const ReloadInput &reloadInput) const
    {
        if (reloadInput.text[LeftSide] == reloadInput.text[RightSide])
            return; // We show "No difference" in this case, regardless if it's binary or not

        Differ differ(QFuture<void>(promise.future()));

        FileData fileData;
        if (!reloadInput.binaryFiles) {
            const QList<Diff> diffList = Differ::cleanupSemantics(
                        differ.diff(reloadInput.text[LeftSide], reloadInput.text[RightSide]));

            QList<Diff> leftDiffList;
            QList<Diff> rightDiffList;
            Differ::splitDiffList(diffList, &leftDiffList, &rightDiffList);
            QList<Diff> outputLeftDiffList;
            QList<Diff> outputRightDiffList;

            if (m_ignoreWhitespace) {
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
            fileData = DiffUtils::calculateContextData(chunkData, m_contextLineCount, 0);
        }
        fileData.fileInfo = reloadInput.fileInfo;
        fileData.fileOperation = reloadInput.fileOperation;
        fileData.binaryFiles = reloadInput.binaryFiles;
        promise.addResult(fileData);
    }

private:
    const int m_contextLineCount;
    const bool m_ignoreWhitespace;
};

class DiffFilesController : public DiffEditorController
{
    Q_OBJECT
public:
    DiffFilesController(IDocument *document);

protected:
    virtual QList<ReloadInput> reloadInputList() const = 0;
};

DiffFilesController::DiffFilesController(IDocument *document)
    : DiffEditorController(document)
{
    setDisplayName(Tr::tr("Diff"));
    using namespace Tasking;

    const TreeStorage<QList<std::optional<FileData>>> storage;

    const auto setupTree = [this, storage](TaskTree &taskTree) {
        QList<std::optional<FileData>> *outputList = storage.activeStorage();

        const auto setupDiff = [this](Async<FileData> &async, const ReloadInput &reloadInput) {
            async.setConcurrentCallData(
                DiffFile(ignoreWhitespace(), contextLineCount()), reloadInput);
            async.setFutureSynchronizer(ExtensionSystem::PluginManager::futureSynchronizer());
        };
        const auto onDiffDone = [outputList](const Async<FileData> &async, int i) {
            if (async.isResultAvailable())
                (*outputList)[i] = async.result();
        };

        const QList<ReloadInput> inputList = reloadInputList();
        outputList->resize(inputList.size());

        using namespace std::placeholders;
        QList<GroupItem> tasks {parallel, finishAllAndDone};
        for (int i = 0; i < inputList.size(); ++i) {
            tasks.append(AsyncTask<FileData>(std::bind(setupDiff, _1, inputList.at(i)),
                                         std::bind(onDiffDone, _1, i)));
        }
        taskTree.setRecipe(tasks);
    };
    const auto onTreeDone = [this, storage] {
        const QList<std::optional<FileData>> &results = *storage;
        QList<FileData> finalList;
        for (const std::optional<FileData> &result : results) {
            if (result.has_value())
                finalList.append(*result);
        }
        setDiffFiles(finalList);
    };
    const auto onTreeError = [this, storage] {
        setDiffFiles({});
    };

    const Group root = {
        Storage(storage),
        TaskTreeTask(setupTree),
        onGroupDone(onTreeDone),
        onGroupError(onTreeError)
    };
    setReloadRecipe(root);
}

class DiffCurrentFileController : public DiffFilesController
{
    Q_OBJECT
public:
    DiffCurrentFileController(IDocument *document, const QString &fileName)
        : DiffFilesController(document)
        , m_fileName(fileName) {}

protected:
    QList<ReloadInput> reloadInputList() const final;

private:
    const QString m_fileName;
};

QList<ReloadInput> DiffCurrentFileController::reloadInputList() const
{
    QList<ReloadInput> result;

    auto textDocument = qobject_cast<TextDocument *>(
        DocumentModel::documentForFilePath(FilePath::fromString(m_fileName)));

    if (textDocument && textDocument->isModified()) {
        QString errorString;
        TextFileFormat format = textDocument->format();

        QString leftText;
        const TextFileFormat::ReadResult leftResult = TextFileFormat::readFile(
            FilePath::fromString(m_fileName), format.codec, &leftText, &format, &errorString);

        const QString rightText = textDocument->plainText();

        ReloadInput reloadInput;
        reloadInput.text = {leftText, rightText};
        reloadInput.fileInfo = {DiffFileInfo(m_fileName, Tr::tr("Saved")),
                                DiffFileInfo(m_fileName, Tr::tr("Modified"))};
        reloadInput.fileInfo[RightSide].patchBehaviour = DiffFileInfo::PatchEditor;
        reloadInput.binaryFiles = (leftResult == TextFileFormat::ReadEncodingError);

        if (leftResult == TextFileFormat::ReadIOError)
            reloadInput.fileOperation = FileData::NewFile;

        result << reloadInput;
    }

    return result;
}

/////////////////

class DiffOpenFilesController : public DiffFilesController
{
    Q_OBJECT
public:
    DiffOpenFilesController(IDocument *document) : DiffFilesController(document) {}

protected:
    QList<ReloadInput> reloadInputList() const final;
};

QList<ReloadInput> DiffOpenFilesController::reloadInputList() const
{
    QList<ReloadInput> result;

    const QList<IDocument *> openedDocuments = DocumentModel::openedDocuments();

    for (IDocument *doc : openedDocuments) {
        QTC_ASSERT(doc, continue);
        auto textDocument = qobject_cast<TextDocument *>(doc);

        if (textDocument && textDocument->isModified()) {
            QString errorString;
            TextFileFormat format = textDocument->format();

            QString leftText;
            const QString fileName = textDocument->filePath().toString();
            const TextFileFormat::ReadResult leftResult = TextFileFormat::readFile(
                FilePath::fromString(fileName), format.codec, &leftText, &format, &errorString);

            const QString rightText = textDocument->plainText();

            ReloadInput reloadInput;
            reloadInput.text = {leftText, rightText};
            reloadInput.fileInfo = {DiffFileInfo(fileName, Tr::tr("Saved")),
                                    DiffFileInfo(fileName, Tr::tr("Modified"))};
            reloadInput.fileInfo[RightSide].patchBehaviour = DiffFileInfo::PatchEditor;
            reloadInput.binaryFiles = (leftResult == TextFileFormat::ReadEncodingError);

            if (leftResult == TextFileFormat::ReadIOError)
                reloadInput.fileOperation = FileData::NewFile;

            result << reloadInput;
        }
    }

    return result;
}

/////////////////

class DiffModifiedFilesController : public DiffFilesController
{
    Q_OBJECT
public:
    DiffModifiedFilesController(IDocument *document, const QStringList &fileNames)
        : DiffFilesController(document)
        , m_fileNames(fileNames) {}

protected:
    QList<ReloadInput> reloadInputList() const final;

private:
    const QStringList m_fileNames;
};

QList<ReloadInput> DiffModifiedFilesController::reloadInputList() const
{
    QList<ReloadInput> result;

    for (const QString &fileName : m_fileNames) {
        auto textDocument = qobject_cast<TextDocument *>(
            DocumentModel::documentForFilePath(FilePath::fromString(fileName)));

        if (textDocument && textDocument->isModified()) {
            QString errorString;
            TextFileFormat format = textDocument->format();

            QString leftText;
            const QString fileName = textDocument->filePath().toString();
            const TextFileFormat::ReadResult leftResult = TextFileFormat::readFile(
                FilePath::fromString(fileName), format.codec, &leftText, &format, &errorString);

            const QString rightText = textDocument->plainText();

            ReloadInput reloadInput;
            reloadInput.text = {leftText, rightText};
            reloadInput.fileInfo = {DiffFileInfo(fileName, Tr::tr("Saved")),
                                    DiffFileInfo(fileName, Tr::tr("Modified"))};
            reloadInput.fileInfo[RightSide].patchBehaviour = DiffFileInfo::PatchEditor;
            reloadInput.binaryFiles = (leftResult == TextFileFormat::ReadEncodingError);

            if (leftResult == TextFileFormat::ReadIOError)
                reloadInput.fileOperation = FileData::NewFile;

            result << reloadInput;
        }
    }

    return result;
}

/////////////////

class DiffExternalFilesController : public DiffFilesController
{
    Q_OBJECT
public:
    DiffExternalFilesController(IDocument *document, const QString &leftFileName,
                                const QString &rightFileName)
        : DiffFilesController(document)
        , m_leftFileName(leftFileName)
        , m_rightFileName(rightFileName) {}

protected:
    QList<ReloadInput> reloadInputList() const final;

private:
    const QString m_leftFileName;
    const QString m_rightFileName;
};

QList<ReloadInput> DiffExternalFilesController::reloadInputList() const
{
    QString errorString;
    TextFileFormat format;
    format.codec = EditorManager::defaultTextCodec();

    QString leftText;
    QString rightText;

    const TextFileFormat::ReadResult leftResult = TextFileFormat::readFile(
        FilePath::fromString(m_leftFileName), format.codec, &leftText, &format, &errorString);
    const TextFileFormat::ReadResult rightResult = TextFileFormat::readFile(
        FilePath::fromString(m_rightFileName), format.codec, &rightText, &format, &errorString);

    ReloadInput reloadInput;
    reloadInput.text = {leftText, rightText};
    reloadInput.fileInfo[LeftSide].fileName = m_leftFileName;
    reloadInput.fileInfo[RightSide].fileName = m_rightFileName;
    reloadInput.binaryFiles = (leftResult == TextFileFormat::ReadEncodingError
            || rightResult == TextFileFormat::ReadEncodingError);

    const bool leftFileExists = (leftResult != TextFileFormat::ReadIOError);
    const bool rightFileExists = (rightResult != TextFileFormat::ReadIOError);
    if (!leftFileExists && rightFileExists)
        reloadInput.fileOperation = FileData::NewFile;
    else if (leftFileExists && !rightFileExists)
        reloadInput.fileOperation = FileData::DeleteFile;

    QList<ReloadInput> result;
    if (leftFileExists || rightFileExists)
        result << reloadInput;

    return result;
}

/////////////////


static TextDocument *currentTextDocument()
{
    return qobject_cast<TextDocument *>(EditorManager::currentDocument());
}

DiffEditorServiceImpl::DiffEditorServiceImpl() = default;

template <typename Controller, typename... Args>
void reload(const QString &vcsId, const QString &displayName, Args &&...args)
{
    auto const document = qobject_cast<DiffEditorDocument *>(
        DiffEditorController::findOrCreateDocument(vcsId, displayName));
    if (!document)
        return;
    if (!DiffEditorController::controller(document))
        new Controller(document, std::forward<Args>(args)...);
    EditorManager::activateEditorForDocument(document);
    document->reload();
}

void DiffEditorServiceImpl::diffFiles(const QString &leftFileName, const QString &rightFileName)
{
    const QString documentId = Constants::DIFF_EDITOR_PLUGIN
            + QLatin1String(".DiffFiles.") + leftFileName + QLatin1Char('.') + rightFileName;
    const QString title = Tr::tr("Diff Files");
    reload<DiffExternalFilesController>(documentId, title, leftFileName, rightFileName);
}

void DiffEditorServiceImpl::diffModifiedFiles(const QStringList &fileNames)
{
    const QString documentId = Constants::DIFF_EDITOR_PLUGIN + QLatin1String(".DiffModifiedFiles");
    const QString title = Tr::tr("Diff Modified Files");
    reload<DiffModifiedFilesController>(documentId, title, fileNames);
}

class DiffEditorPluginPrivate : public QObject
{
public:
    DiffEditorPluginPrivate();

    void updateDiffCurrentFileAction();
    void updateDiffOpenFilesAction();
    void diffCurrentFile();
    void diffOpenFiles();
    void diffExternalFiles();

    QAction *m_diffCurrentFileAction = nullptr;
    QAction *m_diffOpenFilesAction = nullptr;

    DiffEditorFactory m_editorFactory;
    DiffEditorServiceImpl m_service;
};

DiffEditorPluginPrivate::DiffEditorPluginPrivate()
{
    //register actions
    ActionContainer *toolsContainer = ActionManager::actionContainer(Core::Constants::M_TOOLS);
    toolsContainer->insertGroup(Core::Constants::G_TOOLS_DEBUG, Constants::G_TOOLS_DIFF);
    ActionContainer *diffContainer = ActionManager::createMenu("Diff");
    diffContainer->menu()->setTitle(Tr::tr("&Diff"));
    toolsContainer->addMenu(diffContainer, Constants::G_TOOLS_DIFF);

    m_diffCurrentFileAction = new QAction(Tr::tr("Diff Current File"), this);
    Command *diffCurrentFileCommand = ActionManager::registerAction(m_diffCurrentFileAction, "DiffEditor.DiffCurrentFile");
    diffCurrentFileCommand->setDefaultKeySequence(QKeySequence(useMacShortcuts ? Tr::tr("Meta+H") : Tr::tr("Ctrl+H")));
    connect(m_diffCurrentFileAction, &QAction::triggered, this, &DiffEditorPluginPrivate::diffCurrentFile);
    diffContainer->addAction(diffCurrentFileCommand);

    m_diffOpenFilesAction = new QAction(Tr::tr("Diff Open Files"), this);
    Command *diffOpenFilesCommand = ActionManager::registerAction(m_diffOpenFilesAction, "DiffEditor.DiffOpenFiles");
    diffOpenFilesCommand->setDefaultKeySequence(QKeySequence(useMacShortcuts ? Tr::tr("Meta+Shift+H") : Tr::tr("Ctrl+Shift+H")));
    connect(m_diffOpenFilesAction, &QAction::triggered, this, &DiffEditorPluginPrivate::diffOpenFiles);
    diffContainer->addAction(diffOpenFilesCommand);

    QAction *diffExternalFilesAction = new QAction(Tr::tr("Diff External Files..."), this);
    Command *diffExternalFilesCommand = ActionManager::registerAction(diffExternalFilesAction, "DiffEditor.DiffExternalFiles");
    connect(diffExternalFilesAction, &QAction::triggered, this, &DiffEditorPluginPrivate::diffExternalFiles);
    diffContainer->addAction(diffExternalFilesCommand);

    connect(EditorManager::instance(), &EditorManager::currentEditorChanged,
            this, &DiffEditorPluginPrivate::updateDiffCurrentFileAction);
    connect(EditorManager::instance(), &EditorManager::currentDocumentStateChanged,
            this, &DiffEditorPluginPrivate::updateDiffCurrentFileAction);
    connect(EditorManager::instance(), &EditorManager::editorOpened,
            this, &DiffEditorPluginPrivate::updateDiffOpenFilesAction);
    connect(EditorManager::instance(), &EditorManager::editorsClosed,
            this, &DiffEditorPluginPrivate::updateDiffOpenFilesAction);
    connect(EditorManager::instance(), &EditorManager::documentStateChanged,
            this, &DiffEditorPluginPrivate::updateDiffOpenFilesAction);

    updateDiffCurrentFileAction();
    updateDiffOpenFilesAction();
}

void DiffEditorPluginPrivate::updateDiffCurrentFileAction()
{
    auto textDocument = currentTextDocument();
    const bool enabled = textDocument && textDocument->isModified();
    m_diffCurrentFileAction->setEnabled(enabled);
}

void DiffEditorPluginPrivate::updateDiffOpenFilesAction()
{
    const bool enabled = anyOf(DocumentModel::openedDocuments(), [](IDocument *doc) {
            QTC_ASSERT(doc, return false);
            return doc->isModified() && qobject_cast<TextDocument *>(doc);
        });
    m_diffOpenFilesAction->setEnabled(enabled);
}

void DiffEditorPluginPrivate::diffCurrentFile()
{
    auto textDocument = currentTextDocument();
    if (!textDocument)
        return;

    const QString fileName = textDocument->filePath().toString();
    if (fileName.isEmpty())
        return;

    const QString documentId = Constants::DIFF_EDITOR_PLUGIN + QLatin1String(".Diff.") + fileName;
    const QString title = Tr::tr("Diff \"%1\"").arg(fileName);
    reload<DiffCurrentFileController>(documentId, title, fileName);
}

void DiffEditorPluginPrivate::diffOpenFiles()
{
    const QString documentId = Constants::DIFF_EDITOR_PLUGIN + QLatin1String(".DiffOpenFiles");
    const QString title = Tr::tr("Diff Open Files");
    reload<DiffOpenFilesController>(documentId, title);
}

void DiffEditorPluginPrivate::diffExternalFiles()
{
    const FilePath filePath1 = FileUtils::getOpenFilePath(nullptr, Tr::tr("Select First File for Diff"));
    if (filePath1.isEmpty())
        return;
    if (EditorManager::skipOpeningBigTextFile(filePath1))
        return;
    const FilePath filePath2 = FileUtils::getOpenFilePath(nullptr, Tr::tr("Select Second File for Diff"));
    if (filePath2.isEmpty())
        return;
    if (EditorManager::skipOpeningBigTextFile(filePath2))
        return;

    const QString documentId = QLatin1String(Constants::DIFF_EDITOR_PLUGIN)
            + ".DiffExternalFiles." + filePath1.toString() + '.' + filePath2.toString();
    const QString title = Tr::tr("Diff \"%1\", \"%2\"").arg(filePath1.toString(), filePath2.toString());
    reload<DiffExternalFilesController>(documentId, title, filePath1.toString(), filePath2.toString());
}

static DiffEditorPlugin *s_instance = nullptr;

DiffEditorPlugin::DiffEditorPlugin()
{
    s_instance = this;
}

DiffEditorPlugin::~DiffEditorPlugin()
{
    delete d;
    s_instance = nullptr;
}

void DiffEditorPlugin::initialize()
{
    d = new DiffEditorPluginPrivate;
}

} // namespace DiffEditor::Internal

#ifdef WITH_TESTS

#include <QTest>

#include "diffutils.h"

Q_DECLARE_METATYPE(DiffEditor::ChunkData)
Q_DECLARE_METATYPE(DiffEditor::FileData)
Q_DECLARE_METATYPE(DiffEditor::ChunkSelection)

static inline QString _(const char *string) { return QString::fromLatin1(string); }

void DiffEditor::Internal::DiffEditorPlugin::testMakePatch_data()
{
    QTest::addColumn<ChunkData>("sourceChunk");
    QTest::addColumn<bool>("lastChunk");
    QTest::addColumn<QString>("patchText");

    const QString fileName = "a.txt";
    const QString header = "--- " + fileName + "\n+++ " + fileName + '\n';

    QList<RowData> rows;
    rows << RowData(_("ABCD"), TextLineData::Separator);
    rows << RowData(_("EFGH"));
    ChunkData chunk;
    chunk.rows = rows;
    QString patchText = header + "@@ -1,2 +1,1 @@\n"
                                 "-ABCD\n"
                                 " EFGH\n";
    QTest::newRow("Simple not a last chunk") << chunk
                            << false
                            << patchText;

    ///////////

    // chunk the same here
    patchText = header + "@@ -1,2 +1,1 @@\n"
                         "-ABCD\n"
                         " EFGH\n"
                         "\\ No newline at end of file\n";

    QTest::newRow("Simple last chunk") << chunk
                            << true
                            << patchText;

    ///////////

    rows.clear();
    rows << RowData(_("ABCD"));
    rows << RowData(_(""), TextLineData::Separator);
    chunk.rows = rows;
    patchText = header + "@@ -1,1 +1,1 @@\n"
                         "-ABCD\n"
                         "+ABCD\n"
                         "\\ No newline at end of file\n";

    QTest::newRow("EOL in last line removed") << chunk
                            << true
                            << patchText;

    ///////////

    // chunk the same here
    patchText = header + "@@ -1,2 +1,1 @@\n"
                         " ABCD\n"
                         "-\n";

    QTest::newRow("Last empty line removed") << chunk
                            << false
                            << patchText;

    ///////////

    rows.clear();
    rows << RowData(_("ABCD"));
    rows << RowData(_(""), TextLineData::Separator);
    rows << RowData(_(""), TextLineData::Separator);
    chunk.rows = rows;
    patchText = header + "@@ -1,2 +1,1 @@\n"
                         "-ABCD\n"
                         "-\n"
                         "+ABCD\n"
                         "\\ No newline at end of file\n";

    QTest::newRow("Two last EOLs removed") << chunk
                            << true
                            << patchText;

    ///////////

    rows.clear();
    rows << RowData(_("ABCD"));
    rows << RowData(TextLineData::Separator, _(""));
    chunk.rows = rows;
    patchText = header + "@@ -1,1 +1,1 @@\n"
                         "-ABCD\n"
                         "\\ No newline at end of file\n"
                         "+ABCD\n";

    QTest::newRow("EOL to last line added") << chunk
                            << true
                            << patchText;

    ///////////

    // chunk the same here
    patchText = header + "@@ -1,1 +1,2 @@\n"
                         " ABCD\n"
                         "+\n";

    QTest::newRow("Last empty line added") << chunk
                            << false
                            << patchText;

    ///////////

    rows.clear();
    rows << RowData(_("ABCD"), _("EFGH"));
    chunk.rows = rows;
    patchText = header + "@@ -1,1 +1,1 @@\n"
                         "-ABCD\n"
                         "+EFGH\n";

    QTest::newRow("Last line with a newline modified") << chunk
                            << false
                            << patchText;

    ///////////

    rows.clear();
    rows << RowData(_("ABCD"), _("EFGH"));
    rows << RowData(_(""));
    chunk.rows = rows;
    patchText = header + "@@ -1,2 +1,2 @@\n"
                         "-ABCD\n"
                         "+EFGH\n"
                         " \n";
    QTest::newRow("Not a last line with a newline modified") << chunk
                            << false
                            << patchText;

    ///////////

    rows.clear();
    rows << RowData(_("ABCD"), _("EFGH"));
    chunk.rows = rows;
    patchText = header + "@@ -1,1 +1,1 @@\n"
                         "-ABCD\n"
                         "\\ No newline at end of file\n"
                         "+EFGH\n"
                         "\\ No newline at end of file\n";

    QTest::newRow("Last line without a newline modified") << chunk
                            << true
                            << patchText;

    ///////////

    // chunk the same here
    patchText = header + "@@ -1,1 +1,1 @@\n"
                         "-ABCD\n"
                         "+EFGH\n";
    QTest::newRow("Not a last line without a newline modified") << chunk
                            << false
                            << patchText;

    ///////////

    rows.clear();
    rows << RowData(_("ABCD"), _("EFGH"));
    rows << RowData(_("IJKL"));
    chunk.rows = rows;
    patchText = header + "@@ -1,2 +1,2 @@\n"
                         "-ABCD\n"
                         "+EFGH\n"
                         " IJKL\n"
                         "\\ No newline at end of file\n";

    QTest::newRow("Last but one line modified, last line without a newline")
            << chunk
            << true
            << patchText;

    ///////////

    // chunk the same here
    patchText = header + "@@ -1,2 +1,2 @@\n"
                         "-ABCD\n"
                         "+EFGH\n"
                         " IJKL\n";

    QTest::newRow("Last but one line modified, last line with a newline")
            << chunk
            << false
            << patchText;

    ///////////

    rows.clear();
    rows << RowData(_("ABCD"));
    rows << RowData(TextLineData::Separator, _(""));
    rows << RowData(_(""), _("EFGH"));
    chunk.rows = rows;
    patchText = header + "@@ -1,1 +1,3 @@\n"
                         " ABCD\n"
                         "+\n"
                         "+EFGH\n"
                         "\\ No newline at end of file\n";

    QTest::newRow("Blank line followed by No newline")
            << chunk
            << true
            << patchText;
}

void DiffEditor::Internal::DiffEditorPlugin::testMakePatch()
{
    QFETCH(ChunkData, sourceChunk);
    QFETCH(bool, lastChunk);
    QFETCH(QString, patchText);

    const QString fileName = "a.txt";
    const QString result = DiffUtils::makePatch(sourceChunk, fileName, fileName, lastChunk);

    QCOMPARE(result, patchText);

    const std::optional<QList<FileData>> resultList = DiffUtils::readPatch(result);
    const bool ok = resultList.has_value();

    QVERIFY(ok);
    QCOMPARE(resultList->count(), 1);
    for (int i = 0; i < resultList->count(); i++) {
        const FileData &resultFileData = resultList->at(i);
        QCOMPARE(resultFileData.fileInfo[LeftSide].fileName, fileName);
        QCOMPARE(resultFileData.fileInfo[RightSide].fileName, fileName);
        QCOMPARE(resultFileData.chunks.count(), 1);
        for (int j = 0; j < resultFileData.chunks.count(); j++) {
            const ChunkData &resultChunkData = resultFileData.chunks.at(j);
            QCOMPARE(resultChunkData.startingLineNumber[LeftSide], sourceChunk.startingLineNumber[LeftSide]);
            QCOMPARE(resultChunkData.startingLineNumber[RightSide], sourceChunk.startingLineNumber[RightSide]);
            QCOMPARE(resultChunkData.contextChunk, sourceChunk.contextChunk);
            QCOMPARE(resultChunkData.rows.count(), sourceChunk.rows.count());
            for (int k = 0; k < sourceChunk.rows.count(); k++) {
                const RowData &sourceRowData = sourceChunk.rows.at(k);
                const RowData &resultRowData = resultChunkData.rows.at(k);
                QCOMPARE(resultRowData.equal, sourceRowData.equal);
                QCOMPARE(resultRowData.line[LeftSide].text, sourceRowData.line[LeftSide].text);
                QCOMPARE(resultRowData.line[LeftSide].textLineType, sourceRowData.line[LeftSide].textLineType);
                QCOMPARE(resultRowData.line[RightSide].text, sourceRowData.line[RightSide].text);
                QCOMPARE(resultRowData.line[RightSide].textLineType, sourceRowData.line[RightSide].textLineType);
            }
        }
    }
}

void DiffEditor::Internal::DiffEditorPlugin::testReadPatch_data()
{
    QTest::addColumn<QString>("sourcePatch");
    QTest::addColumn<QList<FileData>>("fileDataList");

    QString patch = "diff --git a/src/plugins/diffeditor/diffeditor.cpp b/src/plugins/diffeditor/diffeditor.cpp\n"
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
                    ;

    FileData fileData1;
    fileData1.fileInfo = {DiffFileInfo("src/plugins/diffeditor/diffeditor.cpp", "eab9e9b"),
                          DiffFileInfo("src/plugins/diffeditor/diffeditor.cpp", "082c135")};
    ChunkData chunkData1;
    chunkData1.startingLineNumber = {186, 186};
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
    fileData2.fileInfo = {DiffFileInfo(_("src/plugins/diffeditor/diffutils.cpp"), _("2f641c9")),
                          DiffFileInfo(_("src/plugins/diffeditor/diffutils.cpp"), _("f8ff795"))};
    ChunkData chunkData2;
    chunkData2.startingLineNumber = {463, 463};
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
    fileData3.fileInfo = {DiffFileInfo("new", "0000000"), DiffFileInfo("new", "257cc56")};
    fileData3.fileOperation = FileData::NewFile;
    ChunkData chunkData3;
    chunkData3.startingLineNumber = {-1, 0};
    QList<RowData> rows3;
    rows3 << RowData(TextLineData::Separator, _("foo"));
    TextLineData textLineData3(TextLineData::TextLine);
    rows3 << RowData(TextLineData::Separator, textLineData3);
    chunkData3.rows = rows3;
    fileData3.chunks << chunkData3;

    FileData fileData4;
    fileData4.fileInfo = {DiffFileInfo("deleted", "257cc56"), DiffFileInfo("deleted", "0000000")};
    fileData4.fileOperation = FileData::DeleteFile;
    ChunkData chunkData4;
    chunkData4.startingLineNumber = {0, -1};
    QList<RowData> rows4;
    rows4 << RowData(_("foo"), TextLineData::Separator);
    TextLineData textLineData4(TextLineData::TextLine);
    rows4 << RowData(textLineData4, TextLineData::Separator);
    chunkData4.rows = rows4;
    fileData4.chunks << chunkData4;

    FileData fileData5;
    fileData5.fileInfo = {DiffFileInfo("empty", "0000000"), DiffFileInfo("empty", "e69de29")};
    fileData5.fileOperation = FileData::NewFile;

    FileData fileData6;
    fileData6.fileInfo = {DiffFileInfo("empty", "e69de29"), DiffFileInfo("empty", "0000000")};
    fileData6.fileOperation = FileData::DeleteFile;

    FileData fileData7;
    fileData7.fileInfo = {DiffFileInfo("file a.txt", "1234567"),
                          DiffFileInfo("file b.txt", "9876543")};
    fileData7.fileOperation = FileData::CopyFile;
    ChunkData chunkData7;
    chunkData7.startingLineNumber = {19, 19};
    QList<RowData> rows7;
    rows7 << RowData(_("A"));
    rows7 << RowData(_("B"), _("C"));
    rows7 << RowData(_("D"));
    chunkData7.rows = rows7;
    fileData7.chunks << chunkData7;

    FileData fileData8;
    fileData8.fileInfo = {DiffFileInfo("file a.txt"), DiffFileInfo("file b.txt")};
    fileData8.fileOperation = FileData::RenameFile;

    FileData fileData9;
    fileData9.fileInfo = {DiffFileInfo("file.txt", "1234567"), DiffFileInfo("file.txt", "9876543")};
    fileData9.chunks << chunkData7;
    QList<FileData> fileDataList1;
    fileDataList1 << fileData1 << fileData2 << fileData3 << fileData4 << fileData5
                  << fileData6 << fileData7 << fileData8 << fileData9;

    QTest::newRow("Git patch") << patch
                               << fileDataList1;

    //////////////

    patch = "diff --git a/file foo.txt b/file foo.txt\n"
            "index 1234567..9876543 100644\n"
            "--- a/file foo.txt\n"
            "+++ b/file foo.txt\n"
            "@@ -50,4 +50,5 @@ void DiffEditor::ctor()\n"
            " A\n"
            " B\n"
            " C\n"
            "+\n";

    fileData1.fileInfo = {DiffFileInfo("file foo.txt", "1234567"),
                          DiffFileInfo("file foo.txt", "9876543")};
    fileData1.fileOperation = FileData::ChangeFile;
    chunkData1.startingLineNumber = {49, 49};
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

    patch = "diff --git a/file foo.txt b/file foo.txt\n"
            "index 1234567..9876543 100644\n"
            "--- a/file foo.txt\n"
            "+++ b/file foo.txt\n"
            "@@ -1,1 +1,1 @@\n"
            "-ABCD\n"
            "\\ No newline at end of file\n"
            "+ABCD\n";

    fileData1.fileInfo = {DiffFileInfo("file foo.txt", "1234567"),
                          DiffFileInfo("file foo.txt", "9876543")};
    fileData1.fileOperation = FileData::ChangeFile;
    chunkData1.startingLineNumber = {0, 0};
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

    patch = "diff --git a/difftest.txt b/difftest.txt\n"
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
            ;

    fileData1.fileInfo = {DiffFileInfo("difftest.txt", "1234567"),
                          DiffFileInfo("difftest.txt", "9876543")};
    fileData1.fileOperation = FileData::ChangeFile;
    chunkData1.startingLineNumber = {1, 1};
    rows1.clear();
    rows1 << RowData(_("A"));
    rows1 << RowData(_("B"));
    rows1 << RowData(_("C"), _("Z"));
    rows1 << RowData(_("D"));
    rows1 << RowData(_(""));
    chunkData1.rows = rows1;

    chunkData2.startingLineNumber = {8, 8};
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

    patch = "diff --git a/file foo.txt b/file foo.txt\n"
            "index 1234567..9876543 100644\n"
            "--- a/file foo.txt\n"
            "+++ b/file foo.txt\n"
            "@@ -1,1 +1,3 @@ void DiffEditor::ctor()\n"
            " ABCD\n"
            "+\n"
            "+EFGH\n"
            "\\ No newline at end of file\n";

    fileData1.fileInfo = {DiffFileInfo("file foo.txt", "1234567"),
                          DiffFileInfo("file foo.txt", "9876543")};
    fileData1.fileOperation = FileData::ChangeFile;
    chunkData1.startingLineNumber = {0, 0};
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
    patch = "diff --git a/src/plugins/texteditor/basetextdocument.h b/src/plugins/texteditor/textdocument.h\n"
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
            ;

    fileData1 = {};
    fileData1.fileInfo = {DiffFileInfo("src/plugins/texteditor/basetextdocument.h"),
                          DiffFileInfo("src/plugins/texteditor/textdocument.h")};
    fileData1.fileOperation = FileData::RenameFile;
    fileData2 = {};
    fileData2.fileInfo = {DiffFileInfo("src/plugins/texteditor/basetextdocumentlayout.cpp", "0121933"),
                          DiffFileInfo("src/plugins/texteditor/textdocumentlayout.cpp", "01cc3a0")};
    fileData2.fileOperation = FileData::RenameFile;
    chunkData2.startingLineNumber = {1, 1};
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
    patch = "diff --git a/src/shared/qbs b/src/shared/qbs\n"
            "--- a/src/shared/qbs\n"
            "+++ b/src/shared/qbs\n"
            "@@ -1 +1 @@\n"
            "-Subproject commit eda76354077a427d692fee05479910de31040d3f\n"
            "+Subproject commit eda76354077a427d692fee05479910de31040d3f-dirty\n"
            ;
    fileData1 = {};
    fileData1.fileInfo = {DiffFileInfo("src/shared/qbs"), DiffFileInfo("src/shared/qbs")};
    chunkData1.startingLineNumber = {0, 0};
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
    patch = "diff --git a/demos/arthurplugin/arthurplugin.pro b/demos/arthurplugin/arthurplugin.pro\n"
            "new file mode 100644\n"
            "index 0000000..c5132b4\n"
            "--- /dev/null\n"
            "+++ b/demos/arthurplugin/arthurplugin.pro\n"
            "@@ -0,0 +1 @@\n"
            "+XXX\n"
            "diff --git a/demos/arthurplugin/bg1.jpg b/demos/arthurplugin/bg1.jpg\n"
            "new file mode 100644\n"
            "index 0000000..dfc7cee\n"
            "Binary files /dev/null and b/demos/arthurplugin/bg1.jpg differ\n"
            "diff --git a/demos/arthurplugin/flower.jpg b/demos/arthurplugin/flower.jpg\n"
            "new file mode 100644\n"
            "index 0000000..f8e022c\n"
            "Binary files /dev/null and b/demos/arthurplugin/flower.jpg differ\n"
            ;

    fileData1 = {};
    fileData1.fileInfo = {DiffFileInfo("demos/arthurplugin/arthurplugin.pro", "0000000"),
                          DiffFileInfo("demos/arthurplugin/arthurplugin.pro", "c5132b4")};
    fileData1.fileOperation = FileData::NewFile;
    chunkData1 = {};
    chunkData1.startingLineNumber = {-1, 0};
    rows1.clear();
    rows1 << RowData(TextLineData::Separator, _("XXX"));
    rows1 << RowData(TextLineData::Separator, TextLineData(TextLineData::TextLine));
    chunkData1.rows = rows1;
    fileData1.chunks << chunkData1;
    fileData2 = {};
    fileData2.fileInfo = {DiffFileInfo("demos/arthurplugin/bg1.jpg", "0000000"),
                          DiffFileInfo("demos/arthurplugin/bg1.jpg", "dfc7cee")};
    fileData2.fileOperation = FileData::NewFile;
    fileData2.binaryFiles = true;
    fileData3 = {};
    fileData3.fileInfo = {DiffFileInfo("demos/arthurplugin/flower.jpg", "0000000"),
                          DiffFileInfo("demos/arthurplugin/flower.jpg", "f8e022c")};
    fileData3.fileOperation = FileData::NewFile;
    fileData3.binaryFiles = true;

    QList<FileData> fileDataList8;
    fileDataList8 << fileData1 << fileData2 << fileData3;

    QTest::newRow("Binary files") << patch
                                  << fileDataList8;

    //////////////
    patch = "diff --git a/script.sh b/script.sh\n"
            "old mode 100644\n"
            "new mode 100755\n"
            ;

    fileData1 = {};
    fileData1.fileInfo = {DiffFileInfo("script.sh"), DiffFileInfo("script.sh")};
    fileData1.fileOperation = FileData::ChangeMode;

    QList<FileData> fileDataList9;
    fileDataList9 << fileData1;

    QTest::newRow("Mode change") << patch << fileDataList9;

    //////////////
    patch = R"(diff --git a/old.sh b/new.sh
old mode 100644
new mode 100755
similarity index 100%
rename from old.sh
rename to new.sh
)"
            ;

    fileData1 = {};
    fileData1.fileInfo = {DiffFileInfo("old.sh"), DiffFileInfo("new.sh")};
    fileData1.fileOperation = FileData::RenameFile;

    QList<FileData> fileDataList10;
    fileDataList10 << fileData1;

    QTest::newRow("Mode change + rename") << patch << fileDataList10;

    //////////////

    // Subversion New
    patch = "Index: src/plugins/subversion/subversioneditor.cpp\n"
            "===================================================================\n"
            "--- src/plugins/subversion/subversioneditor.cpp\t(revision 0)\n"
            "+++ src/plugins/subversion/subversioneditor.cpp\t(revision 0)\n"
            "@@ -0,0 +125 @@\n\n";
    fileData1 = {};
    fileData1.fileInfo = {DiffFileInfo("src/plugins/subversion/subversioneditor.cpp"),
                          DiffFileInfo("src/plugins/subversion/subversioneditor.cpp")};
    chunkData1 = {};
    chunkData1.startingLineNumber = {-1, 124};
    fileData1.chunks << chunkData1;
    QList<FileData> fileDataList21;
    fileDataList21 << fileData1;
    QTest::newRow("Subversion New") << patch
                                    << fileDataList21;

    //////////////

    // Subversion Deleted
    patch = "Index: src/plugins/subversion/subversioneditor.cpp\n"
            "===================================================================\n"
            "--- src/plugins/subversion/subversioneditor.cpp\t(revision 42)\n"
            "+++ src/plugins/subversion/subversioneditor.cpp\t(working copy)\n"
            "@@ -1,125 +0,0 @@\n\n";
    fileData1 = {};
    fileData1.fileInfo = {DiffFileInfo("src/plugins/subversion/subversioneditor.cpp"),
                          DiffFileInfo("src/plugins/subversion/subversioneditor.cpp")};
    chunkData1 = {};
    chunkData1.startingLineNumber = {0, -1};
    fileData1.chunks << chunkData1;
    QList<FileData> fileDataList22;
    fileDataList22 << fileData1;
    QTest::newRow("Subversion Deleted") << patch
                                        << fileDataList22;

    //////////////

    // Subversion Normal
    patch = "Index: src/plugins/subversion/subversioneditor.cpp\n"
            "===================================================================\n"
            "--- src/plugins/subversion/subversioneditor.cpp\t(revision 42)\n"
            "+++ src/plugins/subversion/subversioneditor.cpp\t(working copy)\n"
            "@@ -120,7 +120,7 @@\n\n";
    fileData1 = {};
    fileData1.fileInfo = {DiffFileInfo("src/plugins/subversion/subversioneditor.cpp"),
                          DiffFileInfo("src/plugins/subversion/subversioneditor.cpp")};
    chunkData1 = {};
    chunkData1.startingLineNumber = {119, 119};
    fileData1.chunks << chunkData1;
    QList<FileData> fileDataList23;
    fileDataList23 << fileData1;
    QTest::newRow("Subversion Normal") << patch
                                       << fileDataList23;
}

void DiffEditor::Internal::DiffEditorPlugin::testReadPatch()
{
    QFETCH(QString, sourcePatch);
    QFETCH(QList<FileData>, fileDataList);

    const std::optional<QList<FileData>> result = DiffUtils::readPatch(sourcePatch);
    const bool ok = result.has_value();

    QVERIFY(ok);
    QCOMPARE(result->count(), fileDataList.count());
    for (int i = 0; i < fileDataList.count(); i++) {
        const FileData &origFileData = fileDataList.at(i);
        const FileData &resultFileData = result->at(i);
        QCOMPARE(resultFileData.fileInfo[LeftSide].fileName, origFileData.fileInfo[LeftSide].fileName);
        QCOMPARE(resultFileData.fileInfo[LeftSide].typeInfo, origFileData.fileInfo[LeftSide].typeInfo);
        QCOMPARE(resultFileData.fileInfo[RightSide].fileName, origFileData.fileInfo[RightSide].fileName);
        QCOMPARE(resultFileData.fileInfo[RightSide].typeInfo, origFileData.fileInfo[RightSide].typeInfo);
        QCOMPARE(resultFileData.chunks.count(), origFileData.chunks.count());
        QCOMPARE(resultFileData.fileOperation, origFileData.fileOperation);
        for (int j = 0; j < origFileData.chunks.count(); j++) {
            const ChunkData &origChunkData = origFileData.chunks.at(j);
            const ChunkData &resultChunkData = resultFileData.chunks.at(j);
            QCOMPARE(resultChunkData.startingLineNumber[LeftSide], origChunkData.startingLineNumber[LeftSide]);
            QCOMPARE(resultChunkData.startingLineNumber[RightSide], origChunkData.startingLineNumber[RightSide]);
            QCOMPARE(resultChunkData.contextChunk, origChunkData.contextChunk);
            QCOMPARE(resultChunkData.rows.count(), origChunkData.rows.count());
            for (int k = 0; k < origChunkData.rows.count(); k++) {
                const RowData &origRowData = origChunkData.rows.at(k);
                const RowData &resultRowData = resultChunkData.rows.at(k);
                QCOMPARE(resultRowData.equal, origRowData.equal);
                QCOMPARE(resultRowData.line[LeftSide].text, origRowData.line[LeftSide].text);
                QCOMPARE(resultRowData.line[LeftSide].textLineType, origRowData.line[LeftSide].textLineType);
                QCOMPARE(resultRowData.line[RightSide].text, origRowData.line[RightSide].text);
                QCOMPARE(resultRowData.line[RightSide].textLineType, origRowData.line[RightSide].textLineType);
            }
        }
    }
}

using ListOfStringPairs = QList<QPair<QString, QString>>;

void DiffEditor::Internal::DiffEditorPlugin::testFilterPatch_data()
{
    QTest::addColumn<ChunkData>("chunk");
    QTest::addColumn<ListOfStringPairs>("rows");
    QTest::addColumn<ChunkSelection>("selection");
    QTest::addColumn<PatchAction>("patchAction");

    auto createChunk = [] {
        ChunkData chunk;
        chunk.contextInfo = "void DiffEditor::ctor()";
        chunk.contextChunk = false;
        chunk.startingLineNumber = {49, 49};
        return chunk;
    };
    auto appendRow = [](ChunkData *chunk, const QString &left, const QString &right) {
        RowData row;
        row.equal = (left == right);
        row.line[LeftSide].text = left;
        row.line[LeftSide].textLineType = left.isEmpty() ? TextLineData::Separator : TextLineData::TextLine;
        row.line[RightSide].text = right;
        row.line[RightSide].textLineType = right.isEmpty() ? TextLineData::Separator : TextLineData::TextLine;
        chunk->rows.append(row);
    };
    ChunkData chunk;
    ListOfStringPairs rows;

    chunk = createChunk();
    appendRow(&chunk, "A", "A"); // 50
    appendRow(&chunk, "",  "B"); // 51 +
    appendRow(&chunk, "C", "C"); // 52
    rows = ListOfStringPairs {
        {"A", "A"},
        {"", "B"},
        {"C", "C"}
    };
    QTest::newRow("one added") << chunk << rows << ChunkSelection() << PatchAction::Apply;

    chunk = createChunk();
    appendRow(&chunk, "A", "A"); // 50
    appendRow(&chunk, "B", "");  // 51 -
    appendRow(&chunk, "C", "C"); // 52
    rows = ListOfStringPairs {
        {"A", "A"},
        {"B", ""},
        {"C", "C"}
    };
    QTest::newRow("one removed") << chunk << rows << ChunkSelection() << PatchAction::Apply;

    chunk = createChunk();
    appendRow(&chunk, "A", "A"); // 50
    appendRow(&chunk, "",  "B"); // 51
    appendRow(&chunk, "",  "C"); // 52 +
    appendRow(&chunk, "",  "D"); // 53 +
    appendRow(&chunk, "",  "E"); // 54
    appendRow(&chunk, "F", "F"); // 55
    rows = ListOfStringPairs {
        {"A", "A"},
        {"", "C"},
        {"", "D"},
        {"F", "F"}
    };
    QTest::newRow("stage selected added") << chunk << rows << ChunkSelection({2, 3}, {2, 3})
                                          << PatchAction::Apply;

    chunk = createChunk();
    appendRow(&chunk, "A", "A"); // 50
    appendRow(&chunk, "",  "B"); // 51 +
    appendRow(&chunk, "C", "D"); // 52
    appendRow(&chunk, "E", "E"); // 53
    rows = ListOfStringPairs {
        {"A", "A"},
        {"", "B"},
        {"C", "C"},
        {"E", "E"}
    };
    QTest::newRow("stage selected added keep changed") << chunk << rows << ChunkSelection({1}, {1})
                                                       << PatchAction::Apply;

    chunk = createChunk();
    appendRow(&chunk, "A", "A"); // 50
    appendRow(&chunk, "B", "");  // 51
    appendRow(&chunk, "C", "");  // 52 -
    appendRow(&chunk, "D", "");  // 53 -
    appendRow(&chunk, "E", "");  // 54
    appendRow(&chunk, "F", "F"); // 55
    rows = ListOfStringPairs {
        {"A", "A"},
        {"B", "B"},
        {"C", ""},
        {"D", ""},
        {"E", "E"},
        {"F", "F"}
    };
    QTest::newRow("stage selected removed") << chunk << rows << ChunkSelection({2, 3}, {2, 3})
                                            << PatchAction::Apply;

    chunk = createChunk();
    appendRow(&chunk, "A", "A"); // 50
    appendRow(&chunk, "B", "");  // 51
    appendRow(&chunk, "C", "");  // 52 -
    appendRow(&chunk, "",  "D"); // 53 +
    appendRow(&chunk, "",  "E"); // 54
    appendRow(&chunk, "F", "F"); // 55
    rows = ListOfStringPairs {
        {"A", "A"},
        {"B", "B"},
        {"C", ""},
        {"", "D"},
        {"F", "F"}
    };
    QTest::newRow("stage selected added/removed") << chunk << rows << ChunkSelection({2, 3}, {2, 3})
                                                  << PatchAction::Apply;

    chunk = createChunk();
    appendRow(&chunk, "A", "A"); // 50
    appendRow(&chunk, "B", "C"); // 51 -/+
    appendRow(&chunk, "D", "D"); // 52
    rows = ListOfStringPairs {
        {"A", "A"},
        {"B", "C"},
        {"D", "D"}
    };
    QTest::newRow("stage modified row") << chunk << rows << ChunkSelection({1}, {1})
                                        << PatchAction::Apply;

    chunk = createChunk();
    appendRow(&chunk, "A", "A"); // 50
    appendRow(&chunk, "B", "C"); // 51 -/+
    appendRow(&chunk, "D", "D"); // 52
    rows = ListOfStringPairs {
        {"A", "A"},
        {"B", "C"},
        {"D", "D"}
    };
    QTest::newRow("stage modified and unmodified rows") << chunk << rows
                  << ChunkSelection({0, 1, 2}, {0, 1, 2}) << PatchAction::Apply;

    chunk = createChunk();
    appendRow(&chunk, "A", "A"); // 50
    appendRow(&chunk, "B", "C"); // 51 -/+
    appendRow(&chunk, "D", "D"); // 52
    rows = ListOfStringPairs {
        {"A", "A"},
        {"B", "C"},
        {"D", "D"}
    };
    QTest::newRow("stage unmodified left rows") << chunk << rows << ChunkSelection({0, 1, 2}, {1})
                                                << PatchAction::Apply;

    chunk = createChunk();
    appendRow(&chunk, "A", "A"); // 50
    appendRow(&chunk, "B", "C"); // 51 -/+
    appendRow(&chunk, "D", "D"); // 52
    rows = ListOfStringPairs {
        {"A", "A"},
        {"B", "C"},
        {"D", "D"}
    };
    QTest::newRow("stage unmodified right rows") << chunk << rows << ChunkSelection({1}, {0, 1, 2})
                                                 << PatchAction::Apply;

    chunk = createChunk();
    appendRow(&chunk, "A", "A"); // 50
    appendRow(&chunk, "B", "C"); // 51 -/+
    appendRow(&chunk, "D", "D"); // 52
    rows = ListOfStringPairs {
        {"A", "A"},
        {"B", ""},
        {"D", "D"}
    };
    QTest::newRow("stage left only") << chunk << rows << ChunkSelection({1}, {})
                                     << PatchAction::Apply;

    chunk = createChunk();
    appendRow(&chunk, "A", "A"); // 50
    appendRow(&chunk, "B", "C"); // 51 -/+
    appendRow(&chunk, "D", "D"); // 52
    rows = ListOfStringPairs {
        {"A", "A"},
        {"B", "B"},
        {"", "C"},
        {"D", "D"}
    };
    QTest::newRow("stage right only") << chunk << rows << ChunkSelection({}, {1})
                                      << PatchAction::Apply;

    chunk = createChunk();
    appendRow(&chunk, "A", "A"); // 50
    appendRow(&chunk, "B", "C"); // 51 -/+
    appendRow(&chunk, "D", "D"); // 52
    rows = ListOfStringPairs {
        {"A", "A"},
        {"B", "C"},
        {"D", "D"}
    };
    QTest::newRow("stage modified row and revert") << chunk << rows << ChunkSelection({1}, {1})
                                                   << PatchAction::Revert;

    chunk = createChunk();
    appendRow(&chunk, "A", "A"); // 50
    appendRow(&chunk, "B", "C"); // 51 -/+
    appendRow(&chunk, "D", "D"); // 52
    rows = ListOfStringPairs {
        {"A", "A"},
        {"B", ""},
        {"C", "C"},
        {"D", "D"}
    };
    // symmetric to: "stage right only"
    QTest::newRow("stage left only and revert") << chunk << rows << ChunkSelection({1}, {})
                                                << PatchAction::Revert;

    chunk = createChunk();
    appendRow(&chunk, "A", "A"); // 50
    appendRow(&chunk, "B", "C"); // 51 -/+
    appendRow(&chunk, "D", "D"); // 52
    rows = ListOfStringPairs {
        {"A", "A"},
        {"", "C"},
        {"D", "D"}
    };
    // symmetric to: "stage left only"
    QTest::newRow("stage right only and revert") << chunk << rows << ChunkSelection({}, {1})
                                                 << PatchAction::Revert;
}

void DiffEditor::Internal::DiffEditorPlugin::testFilterPatch()
{
    QFETCH(ChunkData, chunk);
    QFETCH(ListOfStringPairs, rows);
    QFETCH(ChunkSelection, selection);
    QFETCH(PatchAction, patchAction);

    const ChunkData result = DiffEditorDocument::filterChunk(chunk, selection, patchAction);
    QCOMPARE(result.rows.size(), rows.size());
    for (int i = 0; i < rows.size(); ++i) {
        QCOMPARE(result.rows.at(i).line[LeftSide].text, rows.at(i).first);
        QCOMPARE(result.rows.at(i).line[RightSide].text, rows.at(i).second);
    }
}

#endif // WITH_TESTS

#include "diffeditorplugin.moc"
