// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifdef WITH_TESTS

#include "editorconfiguration_test.h"

#include "editorconfiguration.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/icore.h>

#include <texteditor/textdocument.h>
#include <texteditor/texteditor.h>

#include <utils/filepath.h>
#include <utils/qtcsettings.h>
#include <utils/temporarydirectory.h>
#include <utils/textcodec.h>

#include <QScopeGuard>
#include <QPointer>
#include <QSignalSpy>
#include <QTest>

namespace ProjectExplorer {

class EditorConfigurationTest final : public QObject
{
    Q_OBJECT

private slots:
    void testProjectEncodingReplacesFallbackEncoding();
};

void EditorConfigurationTest::testProjectEncodingReplacesFallbackEncoding()
{
    using namespace Utils;
    const QByteArray editorState = Core::EditorManager::saveState();
    QtcSettings *settings = Core::ICore::settings();
    const Key fallbackKey(Core::Constants::SETTINGS_FALLBACKTEXTENCODING);
    const QVariant previousFallback = settings->value(fallbackKey);
    const bool hadFallback = settings->contains(fallbackKey);
    QPointer<TextEditor::TextDocument> document;
    const QScopeGuard restoreSettings([=, &document] {
        if (document)
            Core::EditorManager::closeDocuments({document.data()}, false);
        if (hadFallback)
            settings->setValue(fallbackKey, previousFallback);
        else
            settings->remove(fallbackKey);
        Core::EditorManager::restoreState(editorState);
    });
    settings->setValue(fallbackKey, TextEncoding(TextEncoding::Latin1).name());

    TemporaryDirectory directory("project-editor-fallback-encoding");
    QVERIFY(directory.isValid());
    const FilePath path = directory.filePath("input.txt");
    QVERIFY(path.writeFileContents(QByteArray::fromHex("ff")));

    Core::IEditor *editor = Core::EditorManager::openEditor(path);
    QVERIFY(editor);
    auto *baseEditor = qobject_cast<TextEditor::BaseTextEditor *>(editor);
    QVERIFY(baseEditor);
    document = baseEditor->textDocument();
    QCOMPARE(Core::EditorManager::fallbackTextEncoding(), TextEncoding(TextEncoding::Latin1));
    document->setEncoding(TextEncoding(TextEncoding::Utf8));
    document->setDecodingFallbackEnabled(true);
    QVERIFY(document->isDecodingFallbackEnabled());
    QVERIFY(document->reload());
    QVERIFY(document->decodedWithFallback());
    QVERIFY(!document->isModified());

    EditorConfiguration configuration;
    configuration.useGlobalSettings.setValue(false);
    configuration.setTextEncoding(TextEncoding("Windows-1251"));
    QSignalSpy reloadSpy(document, &Core::IDocument::reloadFinished);
    configuration.configureEditor(editor);

    QCOMPARE(reloadSpy.count(), 1);
    QCOMPARE(document->encoding(), TextEncoding("Windows-1251"));
    QVERIFY(!document->decodedWithFallback());
    QCOMPARE(document->plainText(), QString(QChar(0x044f)));
    QVERIFY(!document->isModified());
}

QObject *createEditorConfigurationTest()
{
    return new EditorConfigurationTest;
}

} // namespace ProjectExplorer

#include "editorconfiguration_test.moc"

#endif // WITH_TESTS
