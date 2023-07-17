// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "texteditorview.h"

#include "texteditorwidget.h"

#include <customnotifications.h>
#include <designmodecontext.h>
#include <designdocument.h>
#include <designersettings.h>
#include <modelnode.h>
#include <model.h>
#include <zoomaction.h>
#include <nodeabstractproperty.h>
#include <nodelistproperty.h>
#include <qmldesignerplugin.h>

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/modemanager.h>

#include <texteditor/texteditor.h>
#include <texteditor/texteditorconstants.h>
#include <qmljseditor/qmljseditordocument.h>

#include <utils/changeset.h>
#include <utils/qtcassert.h>
#include <utils/uniqueobjectptr.h>
#include <qmljs/qmljsmodelmanagerinterface.h>
#include <qmljs/qmljsreformatter.h>

#include <QDebug>
#include <QPair>
#include <QString>
#include <QTimer>
#include <QPointer>

namespace QmlDesigner {

const char TEXTEDITOR_CONTEXT_ID[] = "QmlDesigner.TextEditorContext";

TextEditorView::TextEditorView(ExternalDependenciesInterface &externalDependencies)
    : AbstractView{externalDependencies}
    , m_widget(new TextEditorWidget(this))
    , m_textEditorContext(new Internal::TextEditorContext(m_widget))
{
    Core::ICore::addContextObject(m_textEditorContext);

    Core::Context context(TEXTEDITOR_CONTEXT_ID);

    /*
     * We have to register our own active auto completion shortcut, because the original short cut will
     * use the cursor position of the original editor in the editor manager.
     */

    QAction *completionAction = new QAction(tr("Trigger Completion"), this);
    Core::Command *command = Core::ActionManager::registerAction(completionAction, TextEditor::Constants::COMPLETE_THIS, context);
    command->setDefaultKeySequence(QKeySequence(Core::useMacShortcuts ? tr("Meta+Space") : tr("Ctrl+Space")));

    connect(completionAction, &QAction::triggered, this, [this] {
        if (m_widget->textEditor())
            m_widget->textEditor()->editorWidget()->invokeAssist(TextEditor::Completion);
    });
}

TextEditorView::~TextEditorView()
{
    // m_textEditorContext is responsible for deleting the widget
}

void TextEditorView::modelAttached(Model *model)
{
    Q_ASSERT(model);
    m_widget->clearStatusBar();

    AbstractView::modelAttached(model);

    auto textEditor = Utils::UniqueObjectLatePtr<TextEditor::BaseTextEditor>(
        QmlDesignerPlugin::instance()->currentDesignDocument()->textEditor()->duplicate());

    // Set the context of the text editor, but we add another special context to override shortcuts.
    Core::Context context = textEditor->context();
    context.prepend(TEXTEDITOR_CONTEXT_ID);
    m_textEditorContext->setContext(context);

    m_widget->setTextEditor(std::move(textEditor));
}

void TextEditorView::modelAboutToBeDetached(Model *model)
{
    AbstractView::modelAboutToBeDetached(model);

    m_widget->setTextEditor(nullptr);

    // in case the user closed it explicit we do not want to do anything with the editor
    if (Core::ModeManager::currentModeId() == Core::Constants::MODE_DESIGN) {
        if (TextEditor::BaseTextEditor *textEditor = QmlDesignerPlugin::instance()
                                                         ->currentDesignDocument()
                                                         ->textEditor()) {
            QmlDesignerPlugin::instance()->emitCurrentTextEditorChanged(textEditor);
        }
    }
}

void TextEditorView::importsChanged(const Imports &/*addedImports*/, const Imports &/*removedImports*/)
{
}

void TextEditorView::nodeAboutToBeRemoved(const ModelNode &/*removedNode*/)
{
}

void TextEditorView::rootNodeTypeChanged(const QString &/*type*/, int /*majorVersion*/, int /*minorVersion*/)
{
}

void TextEditorView::propertiesAboutToBeRemoved(const QList<AbstractProperty>& /*propertyList*/)
{
}

void TextEditorView::nodeReparented(const ModelNode &/*node*/, const NodeAbstractProperty &/*newPropertyParent*/, const NodeAbstractProperty &/*oldPropertyParent*/, AbstractView::PropertyChangeFlags /*propertyChange*/)
{
}

WidgetInfo TextEditorView::widgetInfo()
{
    return createWidgetInfo(m_widget,
                            "TextEditor",
                            WidgetInfo::CentralPane,
                            0,
                            tr("Code"),
                            tr("Code view"),
                            DesignerWidgetFlags::IgnoreErrors);
}

void TextEditorView::qmlJSEditorContextHelp(const Core::IContext::HelpCallback &callback) const
{
    if (m_widget->textEditor())
        m_widget->textEditor()->contextHelp(callback);
    else
        callback({});
}

void TextEditorView::nodeIdChanged(const ModelNode& /*node*/, const QString &/*newId*/, const QString &/*oldId*/)
{
}

void TextEditorView::selectedNodesChanged(const QList<ModelNode> &/*selectedNodeList*/,
                                          const QList<ModelNode> &/*lastSelectedNodeList*/)
{
    if (!m_errorState)
        m_widget->jumpTextCursorToSelectedModelNode();
}

void TextEditorView::customNotification(const AbstractView * /*view*/, const QString &identifier, const QList<ModelNode> &/*nodeList*/, const QList<QVariant> &/*data*/)
{
    if (identifier == StartRewriterApply)
        m_widget->setBlockCursorSelectionSynchronisation(true);
    else if (identifier == EndRewriterApply)
        m_widget->setBlockCursorSelectionSynchronisation(false);
}

void TextEditorView::documentMessagesChanged(const QList<DocumentMessage> &errors, const QList<DocumentMessage> &)
{
    if (errors.isEmpty()) {
        m_widget->clearStatusBar();
        m_errorState = false;
    } else {
        const DocumentMessage &error = errors.constFirst();
        m_widget->setStatusText(QString("%1 (Line: %2)").arg(error.description()).arg(error.line()));
        m_errorState = true;
    }
}

bool TextEditorView::changeToMoveTool()
{
    return true;
}

void TextEditorView::changeToDragTool()
{
}

bool TextEditorView::changeToMoveTool(const QPointF &/*beginPoint*/)
{
    return true;
}

void TextEditorView::changeToSelectionTool()
{
}

void TextEditorView::changeToResizeTool()
{
}

void TextEditorView::changeToTransformTools()
{
}

void TextEditorView::changeToCustomTool()
{
}

void TextEditorView::auxiliaryDataChanged(const ModelNode & /*node*/,
                                          AuxiliaryDataKeyView /*type*/,
                                          const QVariant & /*data*/)
{
}

void TextEditorView::instancesCompleted(const QVector<ModelNode> &/*completedNodeList*/)
{
}

void TextEditorView::instanceInformationsChanged(const QMultiHash<ModelNode, InformationName> &/*informationChangeHash*/)
{
}

void TextEditorView::instancesRenderImageChanged(const QVector<ModelNode> &/*nodeList*/)
{
}

void TextEditorView::instancesChildrenChanged(const QVector<ModelNode> &/*nodeList*/)
{
}

void TextEditorView::rewriterBeginTransaction()
{
}

void TextEditorView::rewriterEndTransaction()
{
}

void TextEditorView::gotoCursorPosition(int line, int column)
{
    if (m_widget)
        m_widget->gotoCursorPosition(line, column);
}

void TextEditorView::reformatFile()
{
    QTC_ASSERT(!m_widget.isNull(), return);

    auto document =
            qobject_cast<QmlJSEditor::QmlJSEditorDocument *>(Core::EditorManager::currentDocument());

    // Reformat document if we have a .ui.qml file
    if (document && document->filePath().toString().endsWith(".ui.qml")
                 && QmlDesignerPlugin::settings().value(DesignerSettingsKey::REFORMAT_UI_QML_FILES).toBool()) {

        QmlJS::Document::Ptr currentDocument(document->semanticInfo().document);
        QmlJS::Snapshot snapshot = QmlJS::ModelManagerInterface::instance()->snapshot();

        if (document->isSemanticInfoOutdated()) {
            QmlJS::Document::MutablePtr latestDocument;

            const Utils::FilePath fileName = document->filePath();
            latestDocument = snapshot.documentFromSource(QString::fromUtf8(document->contents()),
                                                         fileName,
                                                         QmlJS::ModelManagerInterface::guessLanguageOfFile(fileName));
            latestDocument->parseQml();
            snapshot.insert(latestDocument);

            currentDocument = latestDocument;
        }

        if (!currentDocument->isParsedCorrectly())
            return;

        const QString &newText = QmlJS::reformat(currentDocument);
        if (currentDocument->source() == newText)
            return;

        QTextCursor tc = m_widget->textEditor()->textCursor();
        int pos = m_widget->textEditor()->textCursor().position();

        Utils::ChangeSet changeSet;
        changeSet.replace(0, document->plainText().length(), newText);

        tc.beginEditBlock();
        changeSet.apply(&tc);
        tc.setPosition(pos);
        tc.endEditBlock();

        m_widget->textEditor()->setTextCursor(tc);
    }
}

void TextEditorView::instancePropertyChanged(const QList<QPair<ModelNode, PropertyName> > &/*propertyList*/)
{
}

} // namespace QmlDesigner
