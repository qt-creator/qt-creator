// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifdef WITH_TESTS

#include "codeassist_test.h"

#include "../texteditor.h"

#include "assistinterface.h"
#include "assistproposaliteminterface.h"
#include "asyncprocessor.h"
#include "completionassistprovider.h"
#include "genericproposal.h"
#include "genericproposalwidget.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>
#include <utils/temporarydirectory.h>

#include <QtTest/QtTest>

namespace TextEditor::Internal {

class TestProposalItem : public AssistProposalItemInterface
{
public:
    QString m_text = "test";
    bool m_implicitlyApplies = false;
    bool m_prematurelyApplies = false;
    QIcon m_icon;
    QString m_detail = "detail";
    bool m_isSnippet = false;
    bool m_isValid = true;

    QString text() const override { return m_text; }
    bool implicitlyApplies() const override { return m_implicitlyApplies; }
    bool prematurelyApplies(const QChar &) const override { return m_prematurelyApplies; }
    QIcon icon() const override { return m_icon; }
    QString detail() const override { return m_detail; }
    bool isSnippet() const override { return m_isSnippet; }
    bool isValid() const override { return m_isValid; }
    quint64 hash() const override { return 0; } // used to remove duplicates
};

class OpenEditorItem : public TestProposalItem
{
public:
    void apply(TextDocumentManipulatorInterface &, int) const override
    {
        m_openedEditor = Core::EditorManager::openEditor(m_filePath,
                                                         Core::Constants::K_DEFAULT_TEXT_EDITOR_ID);
    }

    mutable Core::IEditor *m_openedEditor = nullptr;
    Utils::FilePath m_filePath;
};

class TestProposalWidget : public GenericProposalWidget
{
public:
    void showProposal(const QString &prefix) override
    {
        GenericProposalModelPtr proposalModel = model();
        if (proposalModel && proposalModel->size() == 1) {
            emit proposalItemActivated(proposalModel->proposalItem(0));
            deleteLater();
            return;
        }
        GenericProposalWidget::showProposal(prefix);
    }
};

class TestProposal : public GenericProposal
{
public:
    TestProposal(int pos, const QList<AssistProposalItemInterface *> &items)
        : GenericProposal(pos, items)
    {}
    IAssistProposalWidget *createWidget() const override { return new TestProposalWidget; }
};

class TestProcessor : public AsyncProcessor
{
public:
    TestProcessor(const QList<AssistProposalItemInterface *> &items)
        : m_items(items)
    {}
    IAssistProposal *performAsync() override
    { return new TestProposal(interface()->position(), m_items); }
    QList<AssistProposalItemInterface *> m_items;
};

class TestProvider : public CompletionAssistProvider
{
public:
    IAssistProcessor *createProcessor(const AssistInterface *assistInterface) const override
    {
        Q_UNUSED(assistInterface);
        return new TestProcessor(m_items);
    }
    QList<AssistProposalItemInterface *> m_items;
};

void CodeAssistTests::initTestCase()
{
    Core::IEditor *editor = Core::EditorManager::openEditorWithContents(
        Core::Constants::K_DEFAULT_TEXT_EDITOR_ID);
    QVERIFY(editor);
    m_editor = qobject_cast<BaseTextEditor *>(editor);
    QVERIFY(m_editor);
    m_editorsToClose << m_editor;
    m_testProvider = new TestProvider();
}

static Utils::FilePath createBigFile()
{
    constexpr int textChunkSize = 65536; // from utils/textfileformat.cpp

    const Utils::FilePath result = Utils::TemporaryDirectory::masterDirectoryFilePath() / "BigFile";
    QByteArray data;
    data.reserve(textChunkSize);
    while (data.size() < textChunkSize)
        data.append("bigfile line\n");
    result.writeFileContents(data);
    return result;
}

void CodeAssistTests::testFollowSymbolBigFile()
{
    auto item = new OpenEditorItem;
    item->m_filePath = createBigFile();
    m_testProvider->m_items = {item};
    auto editorWidget = m_editor->editorWidget();

    editorWidget->invokeAssist(FollowSymbol, m_testProvider);
    QSignalSpy spy(editorWidget, &TextEditorWidget::assistFinished);
    QVERIFY(spy.wait(1000));
    QVERIFY(item->m_openedEditor);
    m_editorsToClose << item->m_openedEditor;
}

void CodeAssistTests::cleanupTestCase()
{
    m_testProvider->m_items.clear();
    Core::EditorManager::closeEditors(m_editorsToClose);
    QVERIFY(Core::EditorManager::currentEditor() == nullptr);
}

} // namespace TextEditor::Internal

#endif // ifdef WITH_TESTS
