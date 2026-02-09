// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "texteditorview.h"

#include <assetslibrarywidget.h>
#include <customnotifications.h>
#include <designdocument.h>
#include <designeractionmanager.h>
#include <designersettings.h>
#include <itemlibraryentry.h>
#include <model.h>
#include <modelnode.h>
#include <nodeabstractproperty.h>
#include <nodelistproperty.h>
#include <qmldesignerconstants.h>
#include <qmldesignerplugin.h>
#include <qmlitemnode.h>
#include <qmlstate.h>
#include <rewriterview.h>
#include <texteditorstatusbar.h>
#include <texteditorview.h>
#include <theme.h>
#include <zoomaction.h>

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/findplaceholder.h>
#include <coreplugin/icore.h>
#include <coreplugin/modemanager.h>

#include <qmljseditor/qmljseditordocument.h>

#include <qmljs/qmljsmodelmanagerinterface.h>
#include <qmljs/qmljsreformatter.h>

#include <texteditor/textdocument.h>
#include <texteditor/texteditor.h>
#include <texteditor/texteditorconstants.h>

#include <utils/changeset.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>
#include <utils/uniqueobjectptr.h>

#include <QDebug>
#include <QEvent>
#include <QPair>
#include <QPointer>
#include <QScrollBar>
#include <QTimer>
#include <QVBoxLayout>
#include <QWindow>

#include <algorithm>
#include <memory>
#include <vector>

using namespace Core;

namespace QmlDesigner {

class TextEditorStatusBar;

class TextEditorWidget : public QWidget
{
public:
    TextEditorWidget(TextEditorView *textEditorView);

    void setTextEditor(Utils::UniqueObjectLatePtr<TextEditor::BaseTextEditor> textEditor);

    TextEditor::BaseTextEditor *textEditor() const { return m_textEditor.get(); }

    void contextHelp(const Core::IContext::HelpCallback &callback) const;
    void jumpTextCursorToSelectedModelNode();
    void gotoCursorPosition(int line, int column);

    void setStatusText(const QString &text);
    void clearStatusBar();

    int currentLine() const;

    void setBlockCursorSelectionSynchronisation(bool b);
    void jumpToModelNode(const ModelNode &modelNode);
    void highlightToModelNode(const ModelNode &modelNode);

protected:
    bool eventFilter(QObject *object, QEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *dragEnterEvent) override;
    void dragMoveEvent(QDragMoveEvent *dragMoveEvent) override;
    void dropEvent(QDropEvent *dropEvent) override;

private:
    void updateSelectionByCursorPosition();

    Utils::UniqueObjectLatePtr<TextEditor::BaseTextEditor> m_textEditor;
    QPointer<TextEditorView> m_textEditorView;
    QTimer m_updateSelectionTimer;
    TextEditorStatusBar *m_statusBar = nullptr;
    Core::FindToolBarPlaceHolder *m_findToolBar = nullptr;
    QVBoxLayout *m_layout = nullptr;
    bool m_blockCursorSelectionSynchronisation = false;
    bool m_blockRoundTrip = false;
    ItemLibraryEntry m_draggedEntry;
};

TextEditorWidget::TextEditorWidget(TextEditorView *textEditorView)
    : m_textEditorView(textEditorView)
    , m_statusBar(new TextEditorStatusBar(this))
    , m_findToolBar(new Core::FindToolBarPlaceHolder(this))
    , m_layout(new QVBoxLayout(this))
{
    setAcceptDrops(true);

    m_statusBar->hide();

    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(0);
    m_layout->addWidget(m_statusBar);
    m_layout->addWidget(m_findToolBar);

    m_updateSelectionTimer.setSingleShot(true);
    m_updateSelectionTimer.setInterval(200);

    connect(&m_updateSelectionTimer,
            &QTimer::timeout,
            this,
            &TextEditorWidget::updateSelectionByCursorPosition);
    QmlDesignerPlugin::trackWidgetFocusTime(this, Constants::EVENT_TEXTEDITOR_TIME);
}

void TextEditorWidget::setTextEditor(
    Utils::UniqueObjectLatePtr<TextEditor::BaseTextEditor> textEditor)
{
    std::swap(m_textEditor, textEditor);

    if (m_textEditor) {
        m_layout->insertWidget(0, m_textEditor->editorWidget());

        setFocusProxy(m_textEditor->editorWidget());

        connect(m_textEditor->editorWidget(), &Utils::PlainTextEdit::cursorPositionChanged, this, [this] {
            // Cursor position is changed by rewriter
            if (!m_blockCursorSelectionSynchronisation)
                m_updateSelectionTimer.start();
        });

        m_textEditor->editorWidget()->installEventFilter(this);
    }
}

void TextEditorWidget::contextHelp(const Core::IContext::HelpCallback &callback) const
{
    if (m_textEditorView)
        QmlDesignerPlugin::contextHelp(callback, m_textEditorView->contextHelpId());
    else
        callback({});
}

void TextEditorWidget::updateSelectionByCursorPosition()
{
    if (!m_textEditorView->model())
        return;

    const int cursorPosition = m_textEditor->editorWidget()->textCursor().position();
    RewriterView *rewriterView = m_textEditorView->model()->rewriterView();

    m_blockRoundTrip = true;
    if (rewriterView) {
        ModelNode modelNode = rewriterView->nodeAtTextCursorPosition(cursorPosition);
        if (modelNode.isValid() && !m_textEditorView->isSelectedModelNode(modelNode))
            m_textEditorView->setSelectedModelNode(modelNode);
    }
    m_blockRoundTrip = false;
}

void TextEditorWidget::jumpToModelNode(const ModelNode &modelNode)
{
    RewriterView *rewriterView = m_textEditorView->model()->rewriterView();

    m_blockCursorSelectionSynchronisation = true;
    const int nodeOffset = rewriterView->nodeOffset(modelNode);
    if (nodeOffset > 0) {
        int line, column;
        m_textEditor->editorWidget()->convertPosition(nodeOffset, &line, &column);
        m_textEditor->editorWidget()->gotoLine(line, column);

        highlightToModelNode(modelNode);
    }
    m_blockCursorSelectionSynchronisation = false;
}

void TextEditorWidget::highlightToModelNode(const ModelNode &modelNode)
{
    RewriterView *rewriterView = m_textEditorView->model()->rewriterView();
    const int nodeOffset = rewriterView->nodeOffset(modelNode);
    if (nodeOffset > 0) {
        int line, column;
        m_textEditor->editorWidget()->convertPosition(nodeOffset, &line, &column);

        QTextCursor cursor = m_textEditor->textCursor();
        cursor.setPosition(nodeOffset);
        m_textEditor->editorWidget()->updateFoldingHighlight(cursor);
    }
}

void TextEditorWidget::jumpTextCursorToSelectedModelNode()
{
    if (m_blockRoundTrip)
        return;

    ModelNode selectedNode;

    if (hasFocus())
        return;

    if (m_textEditor && m_textEditor->editorWidget()->hasFocus())
        return;

    if (!m_textEditorView->selectedModelNodes().isEmpty())
        selectedNode = m_textEditorView->selectedModelNodes().constFirst();

    if (selectedNode.isValid()) {
        QmlModelState currentState = m_textEditorView->currentStateNode();
        if (currentState.isBaseState()) {
            jumpToModelNode(selectedNode);
        } else {
            if (currentState.affectsModelNode(selectedNode)) {
                // FIXME: QMLDESIGNER_MERGE
                QTC_CHECK(false);
                // auto propertyChanges = currentState.propertyChanges(selectedNode);
                // jumpToModelNode(propertyChanges.modelNode());
            } else {
                jumpToModelNode(currentState.modelNode());
            }
        }
    }
    m_updateSelectionTimer.stop();
}

void TextEditorWidget::gotoCursorPosition(int line, int column)
{
    if (m_textEditor) {
        m_textEditor->editorWidget()->gotoLine(line, column);
        m_textEditor->editorWidget()->setFocus();
    }
}

void TextEditorWidget::setStatusText(const QString &text)
{
    m_statusBar->setText(text);
    m_statusBar->setVisible(!text.isEmpty());
}

void TextEditorWidget::clearStatusBar()
{
    m_statusBar->clearText();
    m_statusBar->hide();
}

int TextEditorWidget::currentLine() const
{
    if (m_textEditor)
        return m_textEditor->currentLine();
    return -1;
}

void TextEditorWidget::setBlockCursorSelectionSynchronisation(bool b)
{
    m_blockCursorSelectionSynchronisation = b;
}

bool TextEditorWidget::eventFilter(QObject *, QEvent *event)
{
    //do not call the eventfilter when the m_textEditor is gone
    if (!TextEditor::TextEditorWidget::fromEditor(m_textEditor.get()))
        return false;

    static std::vector<int> overrideKeys = { Qt::Key_Delete, Qt::Key_Backspace, Qt::Key_Insert,
                                             Qt::Key_Escape };

    static std::vector<QKeySequence> overrideSequences = {QKeySequence(Qt::CTRL | Qt::ALT),
                                                          QKeySequence(Qt::Key_Left | Qt::CTRL),
                                                          QKeySequence(Qt::Key_Right | Qt::CTRL),
                                                          QKeySequence(Qt::Key_Up | Qt::CTRL),
                                                          QKeySequence(Qt::Key_Down | Qt::CTRL)};
    if (event->type() == QEvent::ShortcutOverride) {
        auto keyEvent = static_cast<QKeyEvent *>(event);

        if (std::find(overrideKeys.begin(), overrideKeys.end(), keyEvent->key()) != overrideKeys.end()) {
            if (keyEvent->key() == Qt::Key_Escape)
                m_findToolBar->hide();

            keyEvent->accept();
            return true;
        }

        static const Qt::KeyboardModifiers relevantModifiers = Qt::ShiftModifier
                                                             | Qt::ControlModifier
                                                             | Qt::AltModifier
                                                             | Qt::MetaModifier;

        const QKeySequence keySqeuence(keyEvent->key() | (keyEvent->modifiers() & relevantModifiers));
        for (const QKeySequence &overrideSequence : overrideSequences) {
            if (keySqeuence.matches(overrideSequence)) {
                keyEvent->accept();
                return true;
            }
        }
    } else if (event->type() == QEvent::FocusIn) {
        m_textEditor->editorWidget()->updateFoldingHighlight(QTextCursor());
    } else if (event->type() == QEvent::FocusOut) {
        m_textEditor->editorWidget()->updateFoldingHighlight(QTextCursor());
    }
    return false;
}

void TextEditorWidget::dragEnterEvent(QDragEnterEvent *dragEnterEvent)
{
    const DesignerActionManager &actionManager
        = QmlDesignerPlugin::instance()->viewManager().designerActionManager();
    if (actionManager.externalDragHasSupportedAssets(dragEnterEvent->mimeData()))
        dragEnterEvent->acceptProposedAction();

    if (dragEnterEvent->mimeData()->hasFormat(Constants::MIME_TYPE_ITEM_LIBRARY_INFO)
        || dragEnterEvent->mimeData()->hasFormat(Constants::MIME_TYPE_ASSETS)) {
        QByteArray data = dragEnterEvent->mimeData()->data(Constants::MIME_TYPE_ITEM_LIBRARY_INFO);
        if (!data.isEmpty()) {
            QDataStream stream(data);
            stream >> m_draggedEntry;
        }
        dragEnterEvent->acceptProposedAction();
    }
}

void TextEditorWidget::dragMoveEvent(QDragMoveEvent *dragMoveEvent)
{
    QTextCursor cursor = m_textEditor->editorWidget()->cursorForPosition(dragMoveEvent->position().toPoint());
    const int cursorPosition = cursor.position();
    RewriterView *rewriterView = m_textEditorView->model()->rewriterView();

    QTC_ASSERT(rewriterView, return );
    ModelNode modelNode = rewriterView->nodeAtTextCursorPosition(cursorPosition);

    if (!modelNode.isValid())
        return;
    highlightToModelNode(modelNode);
}

void TextEditorWidget::dropEvent(QDropEvent *dropEvent)
{
    QTextCursor cursor = m_textEditor->editorWidget()->cursorForPosition(dropEvent->position().toPoint());
    const int cursorPosition = cursor.position();
    RewriterView *rewriterView = m_textEditorView->model()->rewriterView();

    QTC_ASSERT(rewriterView, return);
    ModelNode modelNode = rewriterView->nodeAtTextCursorPosition(cursorPosition);

    if (!modelNode.isValid())
        return;

    auto targetProperty = modelNode.defaultNodeAbstractProperty();

    if (dropEvent->mimeData()->hasFormat(Constants::MIME_TYPE_ITEM_LIBRARY_INFO)) {
        if (!m_draggedEntry.name().isEmpty()) {
            m_textEditorView->executeInTransaction("TextEditorWidget::dropEventItem", [&] {
                auto newQmlObjectNode = QmlItemNode::createQmlObjectNode(m_textEditorView,
                                                                         m_draggedEntry,
                                                                         QPointF(),
                                                                         targetProperty,
                                                                         false);
            });
        }
    } else if (dropEvent->mimeData()->hasFormat(Constants::MIME_TYPE_ASSETS)) {
        const QStringList assetsPaths
            = QString::fromUtf8(dropEvent->mimeData()->data(Constants::MIME_TYPE_ASSETS)).split(',');

        QTC_ASSERT(!assetsPaths.isEmpty(), return);
        const ModelNode targetNode = targetProperty.parentModelNode();
        QList<ModelNode> addedNodes;

        for (const QString &assetPath : assetsPaths) {
            ModelNode newModelNode;
            const auto assetTypeAndData = AssetsLibraryWidget::getAssetTypeAndData(assetPath);
            const QString assetType = assetTypeAndData.first;
            const QString assetData = QString::fromUtf8(assetTypeAndData.second);
            bool moveNodesAfter = true; // Appending to parent is the default in text editor
            if (assetType == Constants::MIME_TYPE_ASSET_IMAGE) {
                newModelNode = ModelNodeOperations::handleItemLibraryImageDrop(assetPath,
                                                                               targetProperty,
                                                                               targetNode,
                                                                               moveNodesAfter);
            } else if (assetType == Constants::MIME_TYPE_ASSET_FONT) {
                newModelNode = ModelNodeOperations::handleItemLibraryFontDrop(
                    assetData, // assetData is fontFamily
                    targetProperty,
                    targetNode);
            } else if (assetType == Constants::MIME_TYPE_ASSET_SHADER) {
                newModelNode = ModelNodeOperations::handleItemLibraryShaderDrop(assetPath,
                                                                                assetData == "f",
                                                                                targetProperty,
                                                                                targetNode,
                                                                                moveNodesAfter);
            } else if (assetType == Constants::MIME_TYPE_ASSET_SOUND) {
                newModelNode = ModelNodeOperations::handleItemLibrarySoundDrop(assetPath,
                                                                               targetProperty,
                                                                               targetNode);
            } else if (assetType == Constants::MIME_TYPE_ASSET_TEXTURE3D) {
                newModelNode = ModelNodeOperations::handleItemLibraryTexture3dDrop(assetPath,
                                                                                   targetNode,
                                                                                   moveNodesAfter);
            } else if (assetType == Constants::MIME_TYPE_ASSET_EFFECT) {
                newModelNode = ModelNodeOperations::handleItemLibraryEffectDrop(assetPath,
                                                                                targetNode);
            }

            if (newModelNode.isValid())
                addedNodes.append(newModelNode);
        }

        if (!addedNodes.isEmpty())
            m_textEditorView->setSelectedModelNodes(addedNodes);
    } else {
        const DesignerActionManager &actionManager
            = QmlDesignerPlugin::instance()->viewManager().designerActionManager();
        actionManager.handleExternalAssetsDrop(dropEvent->mimeData());
    }
    m_textEditorView->model()->endDrag();
    m_textEditor->editorWidget()->updateFoldingHighlight(QTextCursor());
}


// TextEditorView

TextEditorView::TextEditorView(ExternalDependenciesInterface &externalDependencies)
    : AbstractView{externalDependencies}
    , m_widget(new TextEditorWidget(this))
{
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

    createTextEditor();
}

void TextEditorView::modelAboutToBeDetached(Model *model)
{
    AbstractView::modelAboutToBeDetached(model);

    if (m_widget)
        m_widget->setTextEditor(nullptr);
    disconnect(m_designDocumentConnection);
}

WidgetInfo TextEditorView::widgetInfo()
{
    return createWidgetInfo(m_widget,
                            "TextEditor",
                            WidgetInfo::CentralPane,
                            tr("Code"),
                            tr("Code view"),
                            DesignerWidgetFlags::IgnoreErrors);
}

TextEditor::BaseTextEditor *TextEditorView::textEditor()
{
    return m_widget->textEditor();
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
    if (document && document->filePath().toUrlishString().endsWith(".ui.qml")
                 && designerSettings().reformatUiQmlFiles()) {

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

        const bool hasEditor = m_widget->textEditor();
        if (!hasEditor)
            createTextEditor();

        QTextCursor tc = m_widget->textEditor()->textCursor();
        int pos = m_widget->textEditor()->textCursor().position();

        Utils::ChangeSet changeSet;
        changeSet.replace(0, document->plainText().size(), newText);

        tc.beginEditBlock();
        changeSet.apply(&tc);
        tc.setPosition(pos);
        tc.endEditBlock();

        m_widget->textEditor()->setTextCursor(tc);

        if (!hasEditor)
            m_widget->setTextEditor(nullptr);
    }
}

void TextEditorView::jumpToModelNode(const ModelNode &modelNode)
{
    m_widget->jumpToModelNode(modelNode);

    m_widget->window()->windowHandle()->requestActivate();
    m_widget->textEditor()->widget()->setFocus();
    m_widget->textEditor()->editorWidget()->updateFoldingHighlight(QTextCursor());
}

void TextEditorView::createTextEditor()
{
    DesignDocument *designDocument = QmlDesignerPlugin::instance()->currentDesignDocument();
    auto textEditor = Utils::UniqueObjectLatePtr<TextEditor::BaseTextEditor>(
        designDocument->textEditor()->duplicate());
    static constexpr char qmlTextEditorContextId[] = "QmlDesigner::TextEditor";
    IContext::attach(textEditor->widget(),
                     Context(qmlTextEditorContextId, Constants::qtQuickToolsMenuContextId),
                     [this](const IContext::HelpCallback &callback) {
                         m_widget->contextHelp(callback);
                     });
    m_widget->setTextEditor(std::move(textEditor));

    disconnect(m_designDocumentConnection);
    m_designDocumentConnection = connect(designDocument,
                                         &DesignDocument::designDocumentClosed,
                                         m_widget,
                                         [this] { m_widget->setTextEditor(nullptr); });
}

} // namespace QmlDesigner
