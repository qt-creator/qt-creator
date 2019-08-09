/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "bindingeditor.h"

#include <qmldesignerplugin.h>

#include "texteditorview.h"
#include "texteditorwidget.h"
#include <texteditor/texteditor.h>

#include <coreplugin/icore.h>
#include <coreplugin/actionmanager/actionmanager.h>

#include <qmldesigner/qmldesignerplugin.h>
#include <qmldesigner/qmldesignerconstants.h>
#include <qmljseditor/qmljseditor.h>
#include <qmljseditor/qmljseditorconstants.h>
#include <qmljstools/qmljstoolsconstants.h>
#include <qmljseditor/qmljscompletionassist.h>
#include <qmljseditor/qmljshighlighter.h>
#include <qmljseditor/qmljshoverhandler.h>
#include <qmljstools/qmljsindenter.h>
#include <qmljseditor/qmljsautocompleter.h>
#include <qmljseditor/qmljseditordocument.h>
#include <qmljseditor/qmljssemantichighlighter.h>
#include <texteditor/textdocument.h>
#include <texteditor/texteditoractionhandler.h>
#include <texteditor/codeassist/assistinterface.h>
#include <texteditor/codeassist/completionassistprovider.h>
#include <texteditor/syntaxhighlighter.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <coreplugin/editormanager/editormanager.h>

#include <QDialogButtonBox>
#include <QDebug>
#include <QVBoxLayout>

namespace QmlDesigner {

static BindingEditor *s_lastBindingEditor = nullptr;

const char BINDINGEDITOR_CONTEXT_ID[] = "BindingEditor.BindingEditorContext";

BindingEditorWidget::BindingEditorWidget()
    : m_context(new BindingEditorContext(this))
{
    Core::ICore::addContextObject(m_context);

    Core::Context context(BINDINGEDITOR_CONTEXT_ID);

    /*
     * We have to register our own active auto completion shortcut, because the original short cut will
     * use the cursor position of the original editor in the editor manager.
     */

    m_completionAction = new QAction(tr("Trigger Completion"), this);
    Core::Command *command = Core::ActionManager::registerAction(m_completionAction, TextEditor::Constants::COMPLETE_THIS, context);
    command->setDefaultKeySequence(QKeySequence(Core::useMacShortcuts ? tr("Meta+Space") : tr("Ctrl+Space")));

    connect(m_completionAction, &QAction::triggered, [this]() {
        invokeAssist(TextEditor::Completion);
    });
}

BindingEditorWidget::~BindingEditorWidget()
{
    unregisterAutoCompletion();

    Core::ICore::removeContextObject(m_context);
    delete m_context;
}

void BindingEditorWidget::unregisterAutoCompletion()
{
    if (m_completionAction)
    {
        Core::ActionManager::unregisterAction(m_completionAction, TextEditor::Constants::COMPLETE_THIS);
        delete m_completionAction;
        m_completionAction = nullptr;
    }
}

TextEditor::AssistInterface *BindingEditorWidget::createAssistInterface(TextEditor::AssistKind assistKind, TextEditor::AssistReason assistReason) const
{
    Q_UNUSED(assistKind);
    return new QmlJSEditor::QmlJSCompletionAssistInterface(document(), position(), QString(), assistReason, qmljsdocument->semanticInfo());
}

class BindingDocument : public QmlJSEditor::QmlJSEditorDocument
{
public:
    BindingDocument() : m_semanticHighlighter(new QmlJSEditor::SemanticHighlighter(this)) {}
    ~BindingDocument() { delete m_semanticHighlighter; }

protected:
    void applyFontSettings()
    {
        TextDocument::applyFontSettings();
        m_semanticHighlighter->updateFontSettings(fontSettings());
        if (!isSemanticInfoOutdated()) {
            m_semanticHighlighter->rerun(semanticInfo());
        }
    }

   void triggerPendingUpdates()
    {
       TextDocument::triggerPendingUpdates(); // calls applyFontSettings if necessary
       if (!isSemanticInfoOutdated()) {
           m_semanticHighlighter->rerun(semanticInfo());
       }
   }

private:
    QmlJSEditor::SemanticHighlighter *m_semanticHighlighter = nullptr;
};

class BindingEditorFactory : public TextEditor::TextEditorFactory
{
public:
    BindingEditorFactory() {
        setId(BINDINGEDITOR_CONTEXT_ID);
        setDisplayName(QCoreApplication::translate("OpenWith::Editors", BINDINGEDITOR_CONTEXT_ID));


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

    static void decorateEditor(TextEditor::TextEditorWidget *editor)
    {
        editor->textDocument()->setSyntaxHighlighter(new QmlJSEditor::QmlJSHighlighter);
        editor->textDocument()->setIndenter(new QmlJSEditor::Internal::Indenter(editor->textDocument()->document()));
        editor->setAutoCompleter(new QmlJSEditor::AutoCompleter);
    }
};

BindingEditorDialog::BindingEditorDialog(QWidget *parent)
    : QDialog(parent)
    , m_editor(nullptr)
    , m_editorWidget(nullptr)
    , m_verticalLayout(nullptr)
    , m_buttonBox(nullptr)
{
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setWindowTitle(tr("Binding Editor"));
    setModal(false);

    setupJSEditor();
    setupUIComponents();

    QObject::connect(m_buttonBox, &QDialogButtonBox::accepted,
                     this, &BindingEditorDialog::accepted);
    QObject::connect(m_buttonBox, &QDialogButtonBox::rejected,
                     this, &BindingEditorDialog::rejected);
}

BindingEditorDialog::~BindingEditorDialog()
{
    delete m_editor; //m_editorWidget is handled by basetexteditor destructor
    delete m_buttonBox;
    delete m_verticalLayout;
}

void BindingEditorDialog::showWidget(int x, int y)
{
    this->show();
    this->raise();
    move(QPoint(x, y));
    m_editorWidget->setFocus();
}

QString BindingEditorDialog::editorValue() const
{
    if (!m_editorWidget)
        return {};

    return m_editorWidget->document()->toPlainText();
}

void BindingEditorDialog::setEditorValue(const QString &text)
{
    if (m_editorWidget)
        m_editorWidget->document()->setPlainText(text);
}

void BindingEditorDialog::unregisterAutoCompletion()
{
    if (m_editorWidget)
        m_editorWidget->unregisterAutoCompletion();
}

void BindingEditorDialog::setupJSEditor()
{
    static BindingEditorFactory f;
    m_editor = qobject_cast<TextEditor::BaseTextEditor*>(f.createEditor());
    m_editorWidget = qobject_cast<BindingEditorWidget*>(m_editor->editorWidget());

    Core::Context context = m_editor->context();
    context.prepend(BINDINGEDITOR_CONTEXT_ID);
    m_editorWidget->m_context->setContext(context);

    auto qmlDesignerEditor = QmlDesignerPlugin::instance()->currentDesignDocument()->textEditor();

    m_editorWidget->qmljsdocument = qobject_cast<QmlJSEditor::QmlJSEditorWidget *>(
                qmlDesignerEditor->widget())->qmlJsEditorDocument();

    m_editorWidget->setParent(this);

    m_editorWidget->setLineNumbersVisible(false);
    m_editorWidget->setMarksVisible(false);
    m_editorWidget->setCodeFoldingSupported(false);
    m_editorWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_editorWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_editorWidget->setTabChangesFocus(true);
    m_editorWidget->show();
}

void BindingEditorDialog::setupUIComponents()
{
    m_verticalLayout = new QVBoxLayout(this);

    m_buttonBox = new QDialogButtonBox(this);
    m_buttonBox->setOrientation(Qt::Horizontal);
    m_buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);
    m_editorWidget->setFrameStyle(QFrame::StyledPanel | QFrame::Raised);

    m_verticalLayout->addWidget(m_editorWidget);
    m_verticalLayout->addWidget(m_buttonBox);

    this->resize(600,200);
}

BindingEditor::BindingEditor(QObject *)
{
}

BindingEditor::~BindingEditor()
{
    hideWidget();
}

void BindingEditor::registerDeclarativeType()
{
    qmlRegisterType<BindingEditor>("HelperWidgets", 2, 0, "BindingEditor");
}

void BindingEditor::showWidget(int x, int y)
{
    if (s_lastBindingEditor)
        s_lastBindingEditor->hideWidget();
    s_lastBindingEditor = this;

    m_dialog = new BindingEditorDialog(Core::ICore::dialogParent());


    QObject::connect(m_dialog, &BindingEditorDialog::accepted,
                     this, &BindingEditor::accepted);
    QObject::connect(m_dialog, &BindingEditorDialog::rejected,
                     this, &BindingEditor::rejected);

    m_dialog->setAttribute(Qt::WA_DeleteOnClose);
    m_dialog->showWidget(x, y);
}

void BindingEditor::hideWidget()
{
    if (s_lastBindingEditor == this)
        s_lastBindingEditor = nullptr;
    if (m_dialog)
    {
        m_dialog->unregisterAutoCompletion(); //we have to do it separately, otherwise we have an autocompletion action override
        m_dialog->close();
    }
}

QString BindingEditor::bindingValue() const
{
    if (!m_dialog)
        return {};

    return m_dialog->editorValue();
}

void BindingEditor::setBindingValue(const QString &text)
{
    if (m_dialog)
        m_dialog->setEditorValue(text);
}


}
