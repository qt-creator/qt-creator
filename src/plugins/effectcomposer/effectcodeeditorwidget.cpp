// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "effectcodeeditorwidget.h"

#include "effectcomposertr.h"
#include "effectsautocomplete.h"
#include "syntaxhighlighter.h"

#include <qmldesigner/textmodifier/indentingtexteditormodifier.h>

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/coreplugintr.h>
#include <coreplugin/icore.h>

#include <projectexplorer/projectexplorerconstants.h>

#include <qmljseditor/qmljsautocompleter.h>
#include <qmljseditor/qmljscompletionassist.h>
#include <qmljseditor/qmljshighlighter.h>
#include <qmljseditor/qmljshoverhandler.h>
#include <qmljseditor/qmljssemantichighlighter.h>

#include <qmljstools/qmljsindenter.h>

#include <utils/fileutils.h>
#include <utils/mimeconstants.h>
#include <utils/temporaryfile.h>
#include <utils/transientscroll.h>

#include <QAction>
#include <QTemporaryFile>

namespace EffectComposer {

constexpr char EFFECTEDITOR_CONTEXT_ID[] = "EffectEditor.EffectEditorContext";

EffectCodeEditorWidget::EffectCodeEditorWidget()
    : m_context(new Core::IContext(this))
{
    Core::Context context(EFFECTEDITOR_CONTEXT_ID, ProjectExplorer::Constants::QMLJS_LANGUAGE_ID);

    m_context->setWidget(this);
    m_context->setContext(context);
    Core::ICore::addContextObject(m_context);

    Utils::TransientScrollAreaSupport::support(this);

    /*
     * We have to register our own active auto completion shortcut, because the original shortcut will
     * use the cursor position of the original editor in the editor manager.
     */
    m_completionAction = new QAction(Tr::tr("Trigger Completion"), this);

    Core::Command *command = Core::ActionManager::registerAction(
                m_completionAction, TextEditor::Constants::COMPLETE_THIS, context);
    command->setDefaultKeySequence(
        QKeySequence(Core::useMacShortcuts ? Tr::tr("Meta+Space") : Tr::tr("Ctrl+Space")));

    connect(m_completionAction, &QAction::triggered, this, [this] {
        invokeAssist(TextEditor::Completion);
    });

    setLineNumbersVisible(true);
    setMarksVisible(false);
    setCodeFoldingSupported(false);
    setTabChangesFocus(true);
}

EffectCodeEditorWidget::~EffectCodeEditorWidget()
{
    unregisterAutoCompletion();
}

void EffectCodeEditorWidget::unregisterAutoCompletion()
{
    if (m_completionAction) {
        Core::ActionManager::unregisterAction(m_completionAction, TextEditor::Constants::COMPLETE_THIS);
        delete m_completionAction;
        m_completionAction = nullptr;
    }
}

void EffectCodeEditorWidget::setEditorTextWithIndentation(const QString &text)
{
    auto *doc = document();
    doc->setPlainText(text);

    if (Utils::Result<> result = textDocument()->save(); !result)
        qWarning() << __FUNCTION__ << result.error();

    // We don't need to indent an empty text but is also needed for safer text.length()-1 below
    if (text.isEmpty())
        return;

    auto modifier = std::make_unique<QmlDesigner::IndentingTextEditModifier>(doc);
    modifier->indent(0, text.length()-1);
}

std::unique_ptr<TextEditor::AssistInterface> EffectCodeEditorWidget::createAssistInterface(
    [[maybe_unused]] TextEditor::AssistKind assistKind, TextEditor::AssistReason assistReason) const
{
    return std::make_unique<EffectsCompletionAssistInterface>(
        textCursor(),
        Utils::FilePath(),
        assistReason,
        qmlJsEditorDocument()->semanticInfo(),
        getUniforms());
}

void EffectCodeEditorWidget::setUniformsCallback(const std::function<QStringList()> &callback)
{
    m_getUniforms = callback;
}

QStringList EffectCodeEditorWidget::getUniforms() const
{
    if (m_getUniforms)
        return m_getUniforms();
    return {};
}

EffectDocument::EffectDocument()
    : QmlJSEditor::QmlJSEditorDocument(EFFECTEDITOR_CONTEXT_ID)
    , m_semanticHighlighter(new QmlJSEditor::SemanticHighlighter(this))
{}

EffectDocument::~EffectDocument()
{
    delete m_semanticHighlighter;
}

void EffectDocument::applyFontSettings()
{
    TextDocument::applyFontSettings();
    m_semanticHighlighter->updateFontSettings(fontSettings());
    if (!isSemanticInfoOutdated() && semanticInfo().isValid())
        m_semanticHighlighter->rerun(semanticInfo());
}

void EffectDocument::triggerPendingUpdates()
{
    TextDocument::triggerPendingUpdates(); // Calls applyFontSettings if necessary
    if (!isSemanticInfoOutdated() && semanticInfo().isValid())
        m_semanticHighlighter->rerun(semanticInfo());
}

EffectCodeEditorFactory::EffectCodeEditorFactory()
{
    setId(EFFECTEDITOR_CONTEXT_ID);
    setDisplayName(Tr::tr("Effect Code Editor"));
    addMimeType(EFFECTEDITOR_CONTEXT_ID);
    addMimeType(Utils::Constants::QML_MIMETYPE);
    addMimeType(Utils::Constants::QMLTYPES_MIMETYPE);
    addMimeType(Utils::Constants::JS_MIMETYPE);

    setDocumentCreator([]() { return new EffectDocument; });
    setEditorWidgetCreator([]() { return new EffectCodeEditorWidget; });
    setEditorCreator([]() {
        return new QmlJSEditor::QmlJSEditor;
    });
    setAutoCompleterCreator([]() { return new QmlJSEditor::AutoCompleter; });
    setCommentDefinition(Utils::CommentDefinition::CppStyle);
    setParenthesesMatchingEnabled(true);
    setCodeFoldingSupported(true);
    setSyntaxHighlighterCreator([] { return new SyntaxHighlighter; });

    addHoverHandler(new QmlJSEditor::QmlJSHoverHandler);
    setCompletionAssistProvider(new EffectsCompeletionAssistProvider);
}

void EffectCodeEditorFactory::decorateEditor(TextEditor::TextEditorWidget *editor)
{
    editor->textDocument()->setIndenter(QmlJSEditor::createQmlJsIndenter(
                                            editor->textDocument()->document()));

    editor->setLineNumbersVisible(true);
    editor->setMarksVisible(false);
    editor->setCodeFoldingSupported(false);
    editor->setTabChangesFocus(true);
    editor->setRevisionsVisible(true);
}

} // namespace EffectComposer
