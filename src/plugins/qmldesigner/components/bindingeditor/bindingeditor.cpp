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
#include <metainfo.h>
#include <qmlmodelnodeproxy.h>
#include <variantproperty.h>
#include <bindingproperty.h>
#include <nodeabstractproperty.h>
#include <nodelistproperty.h>
#include <propertyeditorvalue.h>

#include <QDialogButtonBox>
#include <QPushButton>
#include <QDebug>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QPlainTextEdit>

namespace QmlDesigner {

static BindingEditor *s_lastBindingEditor = nullptr;

const char BINDINGEDITOR_CONTEXT_ID[] = "BindingEditor.BindingEditorContext";

BindingEditorWidget::BindingEditorWidget()
    : m_context(new BindingEditorContext(this))
{
    Core::ICore::addContextObject(m_context);

    const Core::Context context(BINDINGEDITOR_CONTEXT_ID);

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
    if (m_completionAction) {
        Core::ActionManager::unregisterAction(m_completionAction, TextEditor::Constants::COMPLETE_THIS);
        delete m_completionAction;
        m_completionAction = nullptr;
    }
}

bool BindingEditorWidget::event(QEvent *event)
{
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
            emit returnKeyClicked();
            return true;
        } else {
            return QmlJSEditor::QmlJSEditorWidget::event(event);
        }
    }
    return QmlJSEditor::QmlJSEditorWidget::event(event);
}

TextEditor::AssistInterface *BindingEditorWidget::createAssistInterface(
        TextEditor::AssistKind assistKind, TextEditor::AssistReason assistReason) const
{
    Q_UNUSED(assistKind);
    return new QmlJSEditor::QmlJSCompletionAssistInterface(
                document(), position(), QString(),
                assistReason, qmljsdocument->semanticInfo());
}

class BindingDocument : public QmlJSEditor::QmlJSEditorDocument
{
public:
    BindingDocument()
        : QmlJSEditor::QmlJSEditorDocument(BINDINGEDITOR_CONTEXT_ID)
        , m_semanticHighlighter(new QmlJSEditor::SemanticHighlighter(this)) {}
    ~BindingDocument() { delete m_semanticHighlighter; }

protected:
    void applyFontSettings()
    {
        TextDocument::applyFontSettings();
        m_semanticHighlighter->updateFontSettings(fontSettings());
        if (!isSemanticInfoOutdated())
            m_semanticHighlighter->rerun(semanticInfo());
    }

   void triggerPendingUpdates()
    {
       TextDocument::triggerPendingUpdates(); // calls applyFontSettings if necessary
       if (!isSemanticInfoOutdated())
           m_semanticHighlighter->rerun(semanticInfo());
   }

private:
    QmlJSEditor::SemanticHighlighter *m_semanticHighlighter = nullptr;
};

class BindingEditorFactory : public TextEditor::TextEditorFactory
{
public:
    BindingEditorFactory()
    {
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
        editor->textDocument()->setIndenter(new QmlJSEditor::Internal::Indenter(
                                                editor->textDocument()->document()));
        editor->setAutoCompleter(new QmlJSEditor::AutoCompleter);
    }
};

BindingEditorDialog::BindingEditorDialog(QWidget *parent)
    : QDialog(parent)
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
    QObject::connect(m_editorWidget, &BindingEditorWidget::returnKeyClicked,
                     this, &BindingEditorDialog::accepted);

    QObject::connect(m_comboBoxItem, QOverload<int>::of(&QComboBox::currentIndexChanged),
                     this, &BindingEditorDialog::itemIDChanged);
    QObject::connect(m_comboBoxProperty, QOverload<int>::of(&QComboBox::currentIndexChanged),
                     this, &BindingEditorDialog::propertyIDChanged);
    QObject::connect(m_editorWidget, &QPlainTextEdit::textChanged,
                     this, &BindingEditorDialog::textChanged);
}

BindingEditorDialog::~BindingEditorDialog()
{
    delete m_editor; //m_editorWidget is handled by basetexteditor destructor
    delete m_buttonBox;
    delete m_comboBoxItem;
    delete m_comboBoxProperty;
    delete m_comboBoxLayout;
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

void BindingEditorDialog::setAllBindings(QList<BindingEditorDialog::BindingOption> bindings)
{
    m_lock = true;

    m_bindings = bindings;
    setupComboBoxes();
    adjustProperties();

    m_lock = false;
}

void BindingEditorDialog::adjustProperties()
{
    const QString expression = editorValue();
    QString item;
    QString property;
    QStringList expressionElements = expression.split(".");

    if (!expressionElements.isEmpty()) {
        const int itemIndex = m_bindings.indexOf(expressionElements.at(0));

        if (itemIndex != -1) {
            item = expressionElements.at(0);
            expressionElements.removeFirst();

            if (!expressionElements.isEmpty()) {
                const QString sum = expressionElements.join(".");

                if (m_bindings.at(itemIndex).properties.contains(sum))
                    property = sum;
            }
        }
    }

    if (item.isEmpty()) {
        item = undefinedString;
        if (m_comboBoxItem->findText(item) == -1)
            m_comboBoxItem->addItem(item);
    }
    m_comboBoxItem->setCurrentText(item);

    if (property.isEmpty()) {
        property = undefinedString;
        if (m_comboBoxProperty->findText(property) == -1)
            m_comboBoxProperty->addItem(property);
    }
    m_comboBoxProperty->setCurrentText(property);
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


    m_editorWidget->setLineNumbersVisible(false);
    m_editorWidget->setMarksVisible(false);
    m_editorWidget->setCodeFoldingSupported(false);
    m_editorWidget->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_editorWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_editorWidget->setTabChangesFocus(true);
}

void BindingEditorDialog::setupUIComponents()
{
    m_verticalLayout = new QVBoxLayout(this);
    m_comboBoxLayout = new QHBoxLayout;

    m_comboBoxItem = new QComboBox(this);
    m_comboBoxProperty = new QComboBox(this);

    m_editorWidget->setParent(this);
    m_editorWidget->setFrameStyle(QFrame::StyledPanel | QFrame::Raised);
    m_editorWidget->show();

    m_buttonBox = new QDialogButtonBox(this);
    m_buttonBox->setOrientation(Qt::Horizontal);
    m_buttonBox->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    m_buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);


    m_comboBoxLayout->addWidget(m_comboBoxItem);
    m_comboBoxLayout->addWidget(m_comboBoxProperty);

    m_verticalLayout->addLayout(m_comboBoxLayout);
    m_verticalLayout->addWidget(m_editorWidget);
    m_verticalLayout->addWidget(m_buttonBox);

    this->resize(660, 240);
}

void BindingEditorDialog::setupComboBoxes()
{
    m_comboBoxItem->clear();
    m_comboBoxProperty->clear();

    for (auto bind : m_bindings)
        m_comboBoxItem->addItem(bind.item);
}

void BindingEditorDialog::itemIDChanged(int itemID)
{
    const QString previousProperty = m_comboBoxProperty->currentText();
    m_comboBoxProperty->clear();

    if (m_bindings.size() > itemID && itemID != -1) {
        m_comboBoxProperty->addItems(m_bindings.at(itemID).properties);

        if (!m_lock)
            if (m_comboBoxProperty->findText(previousProperty) != -1)
                m_comboBoxProperty->setCurrentText(previousProperty);

        const int undefinedItem = m_comboBoxItem->findText(undefinedString);
        if ((undefinedItem != -1) && (m_comboBoxItem->itemText(itemID) != undefinedString))
            m_comboBoxItem->removeItem(undefinedItem);
    }
}

void BindingEditorDialog::propertyIDChanged(int propertyID)
{
    const int itemID = m_comboBoxItem->currentIndex();

    if (!m_lock)
        if (!m_comboBoxProperty->currentText().isEmpty() && (m_comboBoxProperty->currentText() != undefinedString))
            setEditorValue(m_comboBoxItem->itemText(itemID) + "." + m_comboBoxProperty->itemText(propertyID));

    const int undefinedProperty = m_comboBoxProperty->findText(undefinedString);
    if ((undefinedProperty != -1) && (m_comboBoxProperty->itemText(propertyID) != undefinedString))
        m_comboBoxProperty->removeItem(undefinedProperty);
}

void BindingEditorDialog::textChanged()
{
    if (m_lock)
        return;

    m_lock = true;
    adjustProperties();
    m_lock = false;
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

void BindingEditor::setBackendValue(const QVariant &backendValue)
{
    if (!backendValue.isNull() && backendValue.isValid()) {
        m_backendValue = backendValue;
        const QObject *backendValueObj = backendValue.value<QObject*>();
        const PropertyEditorValue *propertyEditorValue = qobject_cast<const PropertyEditorValue *>(backendValueObj);

        m_backendValueTypeName = propertyEditorValue->modelNode().metaInfo().propertyTypeName(
                    propertyEditorValue->name());

        emit backendValueChanged();
    }
}

void BindingEditor::setModelNodeBackend(const QVariant &modelNodeBackend)
{
    if (!modelNodeBackend.isNull() && modelNodeBackend.isValid()) {
        m_modelNodeBackend = modelNodeBackend;

        emit modelNodeBackendChanged();
    }
}

void BindingEditor::prepareBindings()
{
    if (m_backendValue.isNull() || m_modelNodeBackend.isNull())
        return;

    if (!(m_backendValue.isValid() && m_modelNodeBackend.isValid()))
        return;

    const auto modelNodeBackendObject = m_modelNodeBackend.value<QObject*>();

    const auto backendObjectCasted =
            qobject_cast<const QmlDesigner::QmlModelNodeProxy *>(modelNodeBackendObject);

    if (backendObjectCasted) {
        const QmlDesigner::ModelNode a = backendObjectCasted->qmlObjectNode().modelNode();
        const QList<QmlDesigner::ModelNode> allNodes = a.view()->allModelNodes();

        QList<BindingEditorDialog::BindingOption> bindings;

        for (auto objnode : allNodes) {
            BindingEditorDialog::BindingOption binding;
            for (auto propertyName : objnode.metaInfo().propertyNames())
                if (m_backendValueTypeName == objnode.metaInfo().propertyTypeName(propertyName))
                    binding.properties.append(QString::fromUtf8(propertyName));

            if (!binding.properties.isEmpty() && objnode.hasId()) {
                binding.item = objnode.displayName();
                bindings.append(binding);
            }
        }

        if (!bindings.isEmpty() && !m_dialog.isNull())
            m_dialog->setAllBindings(bindings);
    }
}

QVariant BindingEditor::backendValue() const
{
    return m_backendValue;
}

QVariant BindingEditor::modelNodeBackend() const
{
    return m_modelNodeBackend;
}


}
