// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "texteditorwidget.h"
#include "utils/uniqueobjectptr.h"

#include <assetslibrarywidget.h>
#include <coreplugin/findplaceholder.h>
#include <qmlstate.h>
#include <rewriterview.h>
#include <texteditorstatusbar.h>
#include <texteditorview.h>

#include <designeractionmanager.h>
#include <nodeabstractproperty.h>
#include <qmldesignerconstants.h>
#include <qmldesignerplugin.h>
#include <qmlitemnode.h>

#include <theme.h>

#include <utils/fileutils.h>

#include <coreplugin/editormanager/editormanager.h>
#include <texteditor/textdocument.h>

#include <QEvent>
#include <QScrollBar>
#include <QVBoxLayout>

#include <algorithm>
#include <vector>

namespace QmlDesigner {

TextEditorWidget::TextEditorWidget(TextEditorView *textEditorView)
    : QWidget()
    , m_textEditorView(textEditorView)
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

        QmlDesignerPlugin::instance()->emitCurrentTextEditorChanged(m_textEditor.get());

        connect(m_textEditor->editorWidget(), &QPlainTextEdit::cursorPositionChanged, this, [this] {
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
                auto propertyChanges = currentState.propertyChanges(selectedNode);
                jumpToModelNode(propertyChanges.modelNode());
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
        auto assetTypeAndData = AssetsLibraryWidget::getAssetTypeAndData(assetsPaths.first());
        QString assetType = assetTypeAndData.first;
        QString assetData = QString::fromUtf8(assetTypeAndData.second);

        ModelNode newModelNode;
        ModelNode targetNode = targetProperty.parentModelNode();
        QList<ModelNode> addedNodes;

        for (const QString &assetPath : assetsPaths) {
            auto assetTypeAndData = AssetsLibraryWidget::getAssetTypeAndData(assetPath);
            QString assetType = assetTypeAndData.first;
            QString assetData = QString::fromUtf8(assetTypeAndData.second);
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
                                                                                   targetProperty,
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

} // namespace QmlDesigner
