// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "formwindowfile.h"
#include "formwindoweditor.h"

#include <coreplugin/documentmanager.h>
#include <coreplugin/editormanager/editormanager.h>

#include <utils/filepath.h>
#include <utils/temporarydirectory.h>

#include <QBuffer>
#include <QDesignerFormWindowInterface>
#include <QScopeGuard>
#include <QTest>
#include <QTextDocument>

using namespace Core;
using namespace Utils;

namespace Designer::Internal {

class FormWindowFileTest final : public QObject
{
    Q_OBJECT

private slots:
    void testSyncOnSave();
};

// Check: Saving a form refreshes its text representation, so editors showing
// the source reflect the saved form immediately, not only after a mode switch.
void FormWindowFileTest::testSyncOnSave()
{
    const FilePath source = FilePath::fromUserInput(
        SRCDIR "/../../../tests/designer/gotoslot_withoutProject/form.ui");
    QVERIFY(source.exists());

    // Copy into a writable location so saving does not touch the tracked data.
    TemporaryDirectory tempDir("qtc-designer-test");
    QVERIFY(tempDir.isValid());
    const FilePath target = tempDir.filePath("form.ui");
    QVERIFY(source.copyFile(target));

    IEditor *editor = EditorManager::openEditor(target);
    QVERIFY(editor);
    const QScopeGuard closeEditor([editor] { EditorManager::closeEditors({editor}, false); });

    auto formEditor = qobject_cast<FormWindowEditor *>(editor);
    QVERIFY(formEditor);
    FormWindowFile *file = formEditor->formWindowFile();
    QVERIFY(file);
    QDesignerFormWindowInterface *form = file->formWindow();
    QVERIFY(form);

    const QString initialText = file->document()->toPlainText();
    QVERIFY(initialText.contains("pushButton"));

    // Change the form the way an edit in the designer does: the form model
    // changes but the text document is not resynced on its own.
    QByteArray modifiedXml = QString(initialText).replace("pushButton", "renamedButton").toUtf8();
    QBuffer buffer(&modifiedXml);
    QVERIFY(buffer.open(QIODevice::ReadOnly));
    QString errorString;
    QVERIFY2(form->setContents(&buffer, &errorString), qPrintable(errorString));

    // The text document is now stale while the form already holds the new name.
    QVERIFY(file->formWindowContents().contains("renamedButton"));
    QCOMPARE(file->document()->toPlainText(), initialText);

    // Saving must refresh the text representation from the form.
    QVERIFY(DocumentManager::saveDocument(file));
    QVERIFY(file->document()->toPlainText().contains("renamedButton"));
    QCOMPARE(file->document()->toPlainText(), file->formWindowContents());
}

QObject *createFormWindowFileTest()
{
    return new FormWindowFileTest;
}

} // namespace Designer::Internal

#include "formwindowfile_test.moc"
