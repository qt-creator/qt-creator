// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "documentmanager_test.h"

#include "coreconstants.h"
#include "documentmanager.h"
#include "editormanager/documentmodel.h"
#include "editormanager/editormanager.h"
#include "editormanager/ieditor.h"
#include "idocument.h"

#include <utils/filepath.h>
#include <utils/globalfilechangeblocker.h>
#include <utils/temporarydirectory.h>

#include <QDateTime>
#include <QFile>
#include <QScopeGuard>
#include <QTest>

using namespace Utils;

namespace Core::Internal {

// In-process ports of the Squish system tests suite_editors/tst_edit_externally
// and tst_delete_externally, which opened a file, changed or removed it on disk,
// and checked how the editor reacted through the "file changed/removed on disk"
// banner.
//
// This test is a friend of DocumentManager so it can drive the reload pipeline
// synchronously - changedFile() records a watched path as the file-system
// watcher would, and checkForReload() then applies the reload behavior - instead
// of relying on the asynchronous watcher and its timer. Depending on the reload
// setting, we assert the outcome DocumentManager reaches: silently reloading a
// changed file, silently closing a removed file's editor, or flagging the
// document as conflicted (showing the banner) when it should ask.

// Changes a file's contents on disk and forces its modification time to advance,
// so that DocumentManager detects the change (detection compares time stamps)
// even on file systems with coarse time stamp granularity.
static bool modifyFileOnDisk(const FilePath &filePath, const QByteArray &contents)
{
    if (!filePath.writeFileContents(contents))
        return false;
    QFile file(filePath.path());
    if (!file.open(QIODevice::ReadWrite))
        return false;
    const bool ok = file.setFileTime(QDateTime::currentDateTime().addSecs(60),
                                     QFileDevice::FileModificationTime);
    file.close();
    return ok;
}

class DocumentManagerTest final : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testReloadUnmodifiedOnExternalChange();
    void testAutoCloseOnExternalRemoval();
    void testAlwaysAskFlagsChangeAsConflicted();
    void testReloadAppliesToEveryChangedFile();
    void testAlwaysAskFlagsEveryChangedFile();
    void testSaveAsRestoresRemovedFile();
    void testBinaryFileClosesOnExternalRemoval();

private:
    // Behave as if the file-system watcher reported the paths and run the reload
    // check immediately (accessible because this class is a friend). Recording
    // several paths before the check is how Creator sees a batch of files that
    // changed while it was not looking.
    static void triggerReloadCheck(const FilePaths &filePaths)
    {
        for (const FilePath &filePath : filePaths)
            DocumentManager::instance()->changedFile(filePath);
        DocumentManager::instance()->checkForReload();
    }

    static void triggerReloadCheck(const FilePath &filePath)
    {
        triggerReloadCheck(FilePaths{filePath});
    }

    // Any setting but ReloadUnmodifiedImmediately makes SystemSettings enable
    // global file change blocker (postpones reload check while app is inactive)
    static void setReloadSetting(IDocument::ReloadSetting setting)
    {
        EditorManager::setReloadSetting(setting);
        GlobalFileChangeBlocker::instance()->setBlockingWhileAppIsInactive(false);
    }

    IDocument::ReloadSetting m_originalReloadSetting = IDocument::AlwaysAsk;
    bool m_originalBlockWhileInactive = true;
};

void DocumentManagerTest::initTestCase()
{
    m_originalReloadSetting = EditorManager::reloadSetting();
    m_originalBlockWhileInactive
        = GlobalFileChangeBlocker::instance()->isBlockingWhileAppIsInactive();
}

void DocumentManagerTest::cleanupTestCase()
{
    EditorManager::setReloadSetting(m_originalReloadSetting);
    GlobalFileChangeBlocker::instance()->setBlockingWhileAppIsInactive(
        m_originalBlockWhileInactive);
}

void DocumentManagerTest::testReloadUnmodifiedOnExternalChange()
{
    setReloadSetting(IDocument::ReloadUnmodifiedImmediately);

    TemporaryDirectory tempDir("qtc-documentmanager-XXXXXX");
    QVERIFY(tempDir.isValid());
    const QScopeGuard closeEditors([] { EditorManager::closeAllEditors(false); });
    const FilePath filePath = tempDir.filePath("test.txt");
    QVERIFY(filePath.writeFileContents("line1\n"));

    IEditor *editor = EditorManager::openEditor(filePath);
    QVERIFY(editor);
    IDocument *document = editor->document();
    QVERIFY(document);
    QCOMPARE(document->contents(), QByteArray("line1\n"));

    // An unmodified document is reloaded silently when the file changes on disk.
    QVERIFY(modifyFileOnDisk(filePath, "line1\naddedLine\n"));
    triggerReloadCheck(filePath);

    QCOMPARE(document->contents(), QByteArray("line1\naddedLine\n"));
    QVERIFY(!document->isModified());
}

void DocumentManagerTest::testAutoCloseOnExternalRemoval()
{
    setReloadSetting(IDocument::ReloadUnmodifiedImmediately);

    TemporaryDirectory tempDir("qtc-documentmanager-XXXXXX");
    QVERIFY(tempDir.isValid());
    const QScopeGuard closeEditors([] { EditorManager::closeAllEditors(false); });
    const FilePath filePath = tempDir.filePath("test.txt");
    QVERIFY(filePath.writeFileContents("line1\n"));

    IEditor *editor = EditorManager::openEditor(filePath);
    QVERIFY(editor);
    IDocument *document = editor->document();
    QVERIFY(document);
    QVERIFY(DocumentModel::openedDocuments().contains(document));

    // An unmodified document whose file is removed is closed silently
    // (cf. QTCREATORBUG-8130). Do not dereference document afterwards.
    QVERIFY(filePath.removeFile());
    triggerReloadCheck(filePath);

    QVERIFY(!DocumentModel::openedDocuments().contains(document));
}

void DocumentManagerTest::testAlwaysAskFlagsChangeAsConflicted()
{
    setReloadSetting(IDocument::AlwaysAsk);

    TemporaryDirectory tempDir("qtc-documentmanager-XXXXXX");
    QVERIFY(tempDir.isValid());
    const QScopeGuard closeEditors([] { EditorManager::closeAllEditors(false); });
    const FilePath filePath = tempDir.filePath("test.txt");
    QVERIFY(filePath.writeFileContents("line1\n"));

    IEditor *editor = EditorManager::openEditor(filePath);
    QVERIFY(editor);
    IDocument *document = editor->document();
    QVERIFY(document);

    // With "Always Ask", an external change is not applied automatically: the
    // document is flagged as conflicted (the banner offering Reload/Close/... is
    // shown) and its contents are left untouched until the user decides.
    QVERIFY(modifyFileOnDisk(filePath, "line1\nchanged\n"));
    triggerReloadCheck(filePath);

    QVERIFY(document->isConflicted());
    QCOMPARE(document->contents(), QByteArray("line1\n"));
}

// Opens two files and returns their documents, with contents "<name>\n".
static bool openTwoFiles(const FilePath &first, const FilePath &second,
                         IDocument **firstDocument, IDocument **secondDocument)
{
    if (!first.writeFileContents("first\n") || !second.writeFileContents("second\n"))
        return false;

    IEditor *firstEditor = EditorManager::openEditor(first);
    IEditor *secondEditor = EditorManager::openEditor(second);
    if (!firstEditor || !secondEditor)
        return false;

    *firstDocument = firstEditor->document();
    *secondDocument = secondEditor->document();
    return *firstDocument && *secondDocument;
}

void DocumentManagerTest::testReloadAppliesToEveryChangedFile()
{
    setReloadSetting(IDocument::ReloadUnmodifiedImmediately);

    TemporaryDirectory tempDir("qtc-documentmanager-XXXXXX");
    QVERIFY(tempDir.isValid());
    const QScopeGuard closeEditors([] { EditorManager::closeAllEditors(false); });
    const FilePath first = tempDir.filePath("first.txt");
    const FilePath second = tempDir.filePath("second.txt");

    IDocument *firstDocument = nullptr;
    IDocument *secondDocument = nullptr;
    QVERIFY(openTwoFiles(first, second, &firstDocument, &secondDocument));

    // Both files changed while Creator was not looking. One pass over the
    // recorded changes has to cover all of them, which is what the banner's
    // "Reload All" ends up doing.
    QVERIFY(modifyFileOnDisk(first, "first\naddedLine\n"));
    QVERIFY(modifyFileOnDisk(second, "second\naddedLine\n"));
    triggerReloadCheck({first, second});

    QCOMPARE(firstDocument->contents(), QByteArray("first\naddedLine\n"));
    QCOMPARE(secondDocument->contents(), QByteArray("second\naddedLine\n"));
}

void DocumentManagerTest::testAlwaysAskFlagsEveryChangedFile()
{
    setReloadSetting(IDocument::AlwaysAsk);

    TemporaryDirectory tempDir("qtc-documentmanager-XXXXXX");
    QVERIFY(tempDir.isValid());
    const QScopeGuard closeEditors([] { EditorManager::closeAllEditors(false); });
    const FilePath first = tempDir.filePath("first.txt");
    const FilePath second = tempDir.filePath("second.txt");

    IDocument *firstDocument = nullptr;
    IDocument *secondDocument = nullptr;
    QVERIFY(openTwoFiles(first, second, &firstDocument, &secondDocument));

    QVERIFY(modifyFileOnDisk(first, "first\nchanged\n"));
    QVERIFY(modifyFileOnDisk(second, "second\nchanged\n"));
    triggerReloadCheck({first, second});

    // Every changed file is offered, not just the one that happens to be
    // current: the conflicted set is what "Reload All" and "Ignore All" act on.
    const QList<IDocument *> conflicted = DocumentManager::conflictedDocuments();
    QVERIFY(conflicted.contains(firstDocument));
    QVERIFY(conflicted.contains(secondDocument));
}

void DocumentManagerTest::testSaveAsRestoresRemovedFile()
{
    setReloadSetting(IDocument::AlwaysAsk);

    TemporaryDirectory tempDir("qtc-documentmanager-XXXXXX");
    QVERIFY(tempDir.isValid());
    const QScopeGuard closeEditors([] { EditorManager::closeAllEditors(false); });
    const FilePath filePath = tempDir.filePath("test.txt");
    QVERIFY(filePath.writeFileContents("line1\n"));

    IEditor *editor = EditorManager::openEditor(filePath);
    QVERIFY(editor);
    IDocument *document = editor->document();
    QVERIFY(document);

    // A removed file leaves the editor open and conflicted, offering to save the
    // contents under a name of the user's choosing.
    QVERIFY(filePath.removeFile());
    triggerReloadCheck(filePath);
    QVERIFY(document->isConflicted());
    QVERIFY(!filePath.exists());

    // What the banner's "Save As..." does once a name has been picked - here the
    // original one, so the file is restored.
    QVERIFY(DocumentManager::saveDocument(document, filePath));

    QVERIFY(filePath.exists());
    const Result<QByteArray> restored = filePath.fileContents();
    QVERIFY(restored);
    QCOMPARE(*restored, QByteArray("line1\n"));
}

void DocumentManagerTest::testBinaryFileClosesOnExternalRemoval()
{
    setReloadSetting(IDocument::ReloadUnmodifiedImmediately);

    TemporaryDirectory tempDir("qtc-documentmanager-XXXXXX");
    QVERIFY(tempDir.isValid());
    const QScopeGuard closeEditors([] { EditorManager::closeAllEditors(false); });
    const FilePath filePath = tempDir.filePath("test.bin");
    QVERIFY(filePath.writeFileContents(QByteArray("\x00\x01\x02\xff\xfe\x00", 6)));

    IEditor *editor = EditorManager::openEditor(filePath);
    QVERIFY(editor);
    IDocument *document = editor->document();
    QVERIFY(document);
    QCOMPARE(document->id(), Id(Constants::K_DEFAULT_BINARY_EDITOR_ID));
    QVERIFY(DocumentModel::openedDocuments().contains(document));

    // The system test covered a binary file too. Whichever editor opens it, the
    // removal has to close it. Do not dereference document afterwards.
    QVERIFY(filePath.removeFile());
    triggerReloadCheck(filePath);

    QVERIFY(!DocumentModel::openedDocuments().contains(document));
}

QObject *createDocumentManagerTest()
{
    return new DocumentManagerTest;
}

} // namespace Core::Internal

#include "documentmanager_test.moc"
