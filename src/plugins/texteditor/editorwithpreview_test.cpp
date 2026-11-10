// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "editorwithpreview_test.h"

#include "editorwithpreview.h"
#include "textdocument.h"

#include <coreplugin/editormanager/ieditorfactory.h>
#include <coreplugin/icore.h>

#include <utils/qtcsettings.h>

#include <QSplitter>
#include <QTest>
#include <QTextBrowser>

#include <memory>

using namespace Core;
using namespace Utils;

namespace TextEditor::Internal {

const char markdownPrefix[] = "Markdown";

// Exercises EditorWithPreview through the Markdown editor, which is its
// simplest subclass: the state of the views, duplication and the rendering of
// a preview that was hidden while the document changed.
class EditorWithPreviewTest final : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        for (const Key &key : settingsKeys())
            m_savedSettings.append(ICore::settings()->value(key));
    }

    void cleanupTestCase()
    {
        const QList<Key> keys = settingsKeys();
        for (int i = 0; i < keys.size(); ++i) {
            if (m_savedSettings.at(i).isValid())
                ICore::settings()->setValue(keys.at(i), m_savedSettings.at(i));
            else
                ICore::settings()->remove(keys.at(i));
        }
    }

    void init()
    {
        // new editors take the views from the settings, start from the defaults
        for (const Key &key : settingsKeys())
            ICore::settings()->remove(key);
    }

    void testSaveAndRestoreState()
    {
        std::unique_ptr<EditorWithPreview> editor = createEditor();
        QVERIFY(editor);
        editor->widget()->show();
        QVERIFY(!isTextEditorRight(editor.get()));
        QVERIFY(!preview(editor.get())->isHidden());

        editor->swapViews();
        editor->togglePreview();
        QVERIFY(isTextEditorRight(editor.get()));
        QVERIFY(preview(editor.get())->isHidden());
        const QByteArray state = editor->saveState();
        editor->widget()->hide();

        // only the state, not the settings that the views were changed in, may
        // carry them over to the other editor
        init();
        std::unique_ptr<EditorWithPreview> other = createEditor();
        QVERIFY(other);
        QVERIFY(!isTextEditorRight(other.get()));
        QVERIFY(!preview(other.get())->isHidden());

        other->restoreState(state);
        QVERIFY(isTextEditorRight(other.get()));
        QVERIFY(preview(other.get())->isHidden());
        QCOMPARE(other->saveState(), state);
    }

    void testDuplicate()
    {
        std::unique_ptr<EditorWithPreview> editor = createEditor();
        QVERIFY(editor);
        editor->textDocument()->setPlainText("# Heading\n\nSome text\n");
        editor->updatePreviewNow();
        QVERIFY(preview(editor.get())->toPlainText().contains("Some text"));

        std::unique_ptr<IEditor> duplicate(editor->duplicate());
        QVERIFY(duplicate);
        auto other = qobject_cast<EditorWithPreview *>(duplicate.get());
        QVERIFY(other);
        QCOMPARE(other->document(), editor->document());
        QVERIFY(preview(other)->toPlainText().contains("Some text"));
    }

    void testShowOutdatedPreview()
    {
        std::unique_ptr<EditorWithPreview> editor = createEditor();
        QVERIFY(editor);
        editor->widget()->show();
        editor->textDocument()->setPlainText("First");
        QTRY_VERIFY(preview(editor.get())->toPlainText().contains("First"));

        editor->togglePreview();
        QVERIFY(preview(editor.get())->isHidden());
        editor->textDocument()->setPlainText("Second");
        // what the update timer does when it fires while the preview is hidden
        editor->updatePreview();
        QVERIFY(preview(editor.get())->toPlainText().contains("First"));

        editor->togglePreview();
        QVERIFY(!preview(editor.get())->isHidden());
        QVERIFY(preview(editor.get())->toPlainText().contains("Second"));
        editor->widget()->hide();
    }

private:
    static QList<Key> settingsKeys()
    {
        return {"Markdown.ShowEditor", "Markdown.ShowPreview", "Markdown.TextEditorRight"};
    }

    static std::unique_ptr<EditorWithPreview> createEditor()
    {
        IEditorFactory *factory = IEditorFactory::editorFactoryForId(
            EditorWithPreview::editorId(markdownPrefix));
        if (!factory)
            return {};
        IEditor *editor = factory->createEditor();
        auto editorWithPreview = qobject_cast<EditorWithPreview *>(editor);
        if (!editorWithPreview)
            delete editor;
        return std::unique_ptr<EditorWithPreview>(editorWithPreview);
    }

    static QTextBrowser *preview(EditorWithPreview *editor)
    {
        return editor->widget()->findChild<QTextBrowser *>();
    }

    static bool isTextEditorRight(EditorWithPreview *editor)
    {
        QTextBrowser *previewWidget = preview(editor);
        auto splitter = qobject_cast<QSplitter *>(previewWidget->parentWidget());
        return splitter && splitter->indexOf(previewWidget) == 0;
    }

    QVariantList m_savedSettings;
};

QObject *createEditorWithPreviewTest()
{
    return new EditorWithPreviewTest;
}

} // namespace TextEditor::Internal

#include "editorwithpreview_test.moc"
