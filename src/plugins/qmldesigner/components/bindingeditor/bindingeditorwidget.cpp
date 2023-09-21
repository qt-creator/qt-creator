// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "bindingeditorwidget.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/coreplugintr.h>
#include <coreplugin/icore.h>
#include <plaintexteditmodifier.h>
#include <qmljseditor/qmljsautocompleter.h>
#include <qmljseditor/qmljscompletionassist.h>
#include <qmljseditor/qmljseditor.h>
#include <qmljseditor/qmljseditordocument.h>
#include <qmljseditor/qmljshighlighter.h>
#include <qmljseditor/qmljshoverhandler.h>
#include <qmljseditor/qmljssemantichighlighter.h>
#include <qmljstools/qmljsindenter.h>
#include <qmljstools/qmljstoolsconstants.h>

#include <projectexplorer/projectexplorerconstants.h>
#include <utils/fancylineedit.h>
#include <utils/transientscroll.h>

#include <QAction>

namespace QmlDesigner {

BindingEditorWidget::BindingEditorWidget()
    : m_context(new Core::IContext(this))
{
    Core::Context context(BINDINGEDITOR_CONTEXT_ID,
                          ProjectExplorer::Constants::QMLJS_LANGUAGE_ID);

    m_context->setWidget(this);
    m_context->setContext(context);
    Core::ICore::addContextObject(m_context);

    Utils::TransientScrollAreaSupport::support(this);

    /*
     * We have to register our own active auto completion shortcut, because the original short cut will
     * use the cursor position of the original editor in the editor manager.
     */
    m_completionAction = new QAction(tr("Trigger Completion"), this);

    Core::Command *command = Core::ActionManager::registerAction(
                m_completionAction, TextEditor::Constants::COMPLETE_THIS, context);
    command->setDefaultKeySequence(QKeySequence(
                                       Core::useMacShortcuts
                                       ? tr("Meta+Space")
                                       : tr("Ctrl+Space")));

    connect(m_completionAction, &QAction::triggered, this, [this]() {
        invokeAssist(TextEditor::Completion);
    });
}

BindingEditorWidget::~BindingEditorWidget()
{
    unregisterAutoCompletion();
}

void BindingEditorWidget::unregisterAutoCompletion()
{
    if (m_completionAction) {
        Core::ActionManager::unregisterAction(m_completionAction, TextEditor::Constants::COMPLETE_THIS);
        delete m_completionAction;
        m_completionAction = nullptr;
    }
}

bool BindingEditorWidget::event(QEvent *event)
{
    if (event->type() == QEvent::KeyPress) {
        const QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        const bool returnPressed = (keyEvent->key() == Qt::Key_Return)
                                   || (keyEvent->key() == Qt::Key_Enter);
        const Qt::KeyboardModifiers mods = keyEvent->modifiers();
        constexpr Qt::KeyboardModifier submitModifier = Qt::ControlModifier;
        const bool submitModed = mods.testFlag(submitModifier);

        if (!m_isMultiline && (returnPressed && !mods)) {
            emit returnKeyClicked();
            return true;
        } else if (m_isMultiline && (returnPressed && submitModed)) {
            emit returnKeyClicked();
            return true;
        }
    }
    return QmlJSEditor::QmlJSEditorWidget::event(event);
}

std::unique_ptr<TextEditor::AssistInterface> BindingEditorWidget::createAssistInterface(
    [[maybe_unused]] TextEditor::AssistKind assistKind, TextEditor::AssistReason assistReason) const
{
    return std::make_unique<QmlJSEditor::QmlJSCompletionAssistInterface>(
        textCursor(), Utils::FilePath(), assistReason, qmljsdocument->semanticInfo());
}

void BindingEditorWidget::setEditorTextWithIndentation(const QString &text)
{
    auto *doc = document();
    doc->setPlainText(text);

    //we don't need to indent an empty text
    //but is also needed for safer text.length()-1 below
    if (text.isEmpty())
        return;

    auto modifier = std::make_unique<IndentingTextEditModifier>(
        doc, QTextCursor{doc});
    modifier->indent(0, text.length()-1);
}

BindingDocument::BindingDocument()
    : QmlJSEditor::QmlJSEditorDocument(BINDINGEDITOR_CONTEXT_ID)
    , m_semanticHighlighter(new QmlJSEditor::SemanticHighlighter(this))
{

}

BindingDocument::~BindingDocument()
{
    delete m_semanticHighlighter;
}

void BindingDocument::applyFontSettings()
{
    TextDocument::applyFontSettings();
    m_semanticHighlighter->updateFontSettings(fontSettings());
    if (!isSemanticInfoOutdated())
        m_semanticHighlighter->rerun(semanticInfo());
}

void BindingDocument::triggerPendingUpdates()
{
    TextDocument::triggerPendingUpdates(); // calls applyFontSettings if necessary
    if (!isSemanticInfoOutdated())
        m_semanticHighlighter->rerun(semanticInfo());
}

BindingEditorFactory::BindingEditorFactory()
{
    setId(BINDINGEDITOR_CONTEXT_ID);
    setDisplayName(::Core::Tr::tr("Binding Editor"));
    setEditorActionHandlers(0);
    addMimeType(BINDINGEDITOR_CONTEXT_ID);
    addMimeType(QmlJSTools::Constants::QML_MIMETYPE);
    addMimeType(QmlJSTools::Constants::QMLTYPES_MIMETYPE);
    addMimeType(QmlJSTools::Constants::JS_MIMETYPE);

    setDocumentCreator([]() { return new BindingDocument; });
    setEditorWidgetCreator([]() { return new BindingEditorWidget; });
    setEditorCreator([]() { return new QmlJSEditor::QmlJSEditor; });
    setAutoCompleterCreator([]() { return new QmlJSEditor::AutoCompleter; });
    setCommentDefinition(Utils::CommentDefinition::CppStyle);
    setParenthesesMatchingEnabled(true);
    setCodeFoldingSupported(true);

    addHoverHandler(new QmlJSEditor::QmlJSHoverHandler);
    setCompletionAssistProvider(new QmlJSEditor::QmlJSCompletionAssistProvider);
}

void BindingEditorFactory::decorateEditor(TextEditor::TextEditorWidget *editor)
{
    editor->textDocument()->setSyntaxHighlighter(new QmlJSEditor::QmlJSHighlighter);
    editor->textDocument()->setIndenter(new QmlJSEditor::Internal::Indenter(
                                            editor->textDocument()->document()));
    editor->setAutoCompleter(new QmlJSEditor::AutoCompleter);
}

} // QmlDesigner namespace
