#include "qmljsoutline.h"
#include "qmloutlinemodel.h"

#include <coreplugin/ifile.h>
#include <QtGui/QVBoxLayout>

using namespace QmlJS;

enum {
    debug = false
};

namespace QmlJSEditor {
namespace Internal {

QmlJSOutlineTreeView::QmlJSOutlineTreeView(QWidget *parent) :
    QTreeView(parent)
{
    // see also CppOutlineTreeView
    setFocusPolicy(Qt::NoFocus);
    setFrameStyle(QFrame::NoFrame);
    setAttribute(Qt::WA_MacShowFocusRect, false);
    setUniformRowHeights(true);
    setHeaderHidden(true);
    setTextElideMode(Qt::ElideNone);
    setIndentation(20);
    setExpandsOnDoubleClick(false);
}

QmlJSOutlineWidget::QmlJSOutlineWidget(QWidget *parent) :
    TextEditor::IOutlineWidget(parent),
    m_treeView(new QmlJSOutlineTreeView()),
    m_model(new QmlOutlineModel),
    m_enableCursorSync(true),
    m_blockCursorSync(false)
{
    QVBoxLayout *layout = new QVBoxLayout;

    layout->setMargin(0);
    layout->setSpacing(0);
    layout->addWidget(m_treeView);

    setLayout(layout);

    m_treeView->setModel(m_model);

    connect(m_treeView->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            this, SLOT(updateSelectionInText(QItemSelection)));
}

void QmlJSOutlineWidget::setEditor(QmlJSTextEditor *editor)
{
    m_editor = editor;

    connect(m_editor.data(), SIGNAL(semanticInfoUpdated(QmlJSEditor::Internal::SemanticInfo)),
            this, SLOT(updateOutline(QmlJSEditor::Internal::SemanticInfo)));
    connect(m_editor.data(), SIGNAL(cursorPositionChanged()),
            this, SLOT(updateSelectionInTree()));

    updateOutline(m_editor.data()->semanticInfo());
}

void QmlJSOutlineWidget::setCursorSynchronization(bool syncWithCursor)
{
    m_enableCursorSync = syncWithCursor;
    if (m_enableCursorSync)
        updateSelectionInTree();
}

void QmlJSOutlineWidget::updateOutline(const QmlJSEditor::Internal::SemanticInfo &semanticInfo)
{
    Document::Ptr doc = semanticInfo.document;

    if (!doc) {
        return;
    }

    if (!m_editor
            || m_editor.data()->file()->fileName() != doc->fileName()
            || m_editor.data()->documentRevision() != doc->editorRevision()) {
        return;
    }

    if (doc->ast()
            && m_model) {

        // got a correctly parsed (or recovered) file.

        if (QmlOutlineModel *qmlModel = qobject_cast<QmlOutlineModel*>(m_model)) {
            qmlModel->update(doc);
        }
    } else {
        // TODO: Maybe disable view?
    }

    m_treeView->expandAll();
}

QModelIndex QmlJSOutlineWidget::indexForPosition(const QModelIndex &rootIndex, int cursorPosition)
{
    if (!rootIndex.isValid())
        return QModelIndex();

    AST::SourceLocation location = rootIndex.data(QmlOutlineModel::SourceLocationRole).value<AST::SourceLocation>();

    if (!offsetInsideLocation(cursorPosition, location)) {
        return QModelIndex();
    }

    const int rowCount = rootIndex.model()->rowCount(rootIndex);
    for (int i = 0; i < rowCount; ++i) {
        QModelIndex childIndex = rootIndex.child(i, 0);
        QModelIndex resultIndex = indexForPosition(childIndex, cursorPosition);
        if (resultIndex.isValid())
            return resultIndex;
    }

    return rootIndex;
}

void QmlJSOutlineWidget::updateSelectionInTree()
{
    if (!syncCursor())
        return;

    int absoluteCursorPos = m_editor.data()->textCursor().position();
    QModelIndex index = indexForPosition(m_model->index(0, 0), absoluteCursorPos);

    m_blockCursorSync = true;
    m_treeView->selectionModel()->select(index, QItemSelectionModel::ClearAndSelect);
    m_treeView->scrollTo(index);
    m_blockCursorSync = false;
}

void QmlJSOutlineWidget::updateSelectionInText(const QItemSelection &selection)
{
    if (!syncCursor())
        return;


    if (!selection.indexes().isEmpty()) {
        QModelIndex index = selection.indexes().first();
        AST::SourceLocation location = index.data(QmlOutlineModel::SourceLocationRole).value<AST::SourceLocation>();

        QTextCursor textCursor = m_editor.data()->textCursor();
        m_blockCursorSync = true;
        textCursor.setPosition(location.offset);
        m_editor.data()->setTextCursor(textCursor);
        m_blockCursorSync = false;
    }
}

bool QmlJSOutlineWidget::offsetInsideLocation(quint32 offset, const QmlJS::AST::SourceLocation &location)
{
    return ((offset >= location.offset)
            && (offset <= location.offset + location.length));
}

bool QmlJSOutlineWidget::syncCursor()
{
    return m_enableCursorSync && !m_blockCursorSync;
}

bool QmlJSOutlineWidgetFactory::supportsEditor(Core::IEditor *editor) const
{
    if (qobject_cast<QmlJSEditorEditable*>(editor))
        return true;
    return false;
}

TextEditor::IOutlineWidget *QmlJSOutlineWidgetFactory::createWidget(Core::IEditor *editor)
{
    QmlJSOutlineWidget *widget = new QmlJSOutlineWidget;

    QmlJSEditorEditable *qmlJSEditable = qobject_cast<QmlJSEditorEditable*>(editor);
    QmlJSTextEditor *qmlJSEditor = qobject_cast<QmlJSTextEditor*>(qmlJSEditable->widget());
    Q_ASSERT(qmlJSEditor);

    widget->setEditor(qmlJSEditor);

    return widget;
}

} // namespace Internal
} // namespace QmlJSEditor
