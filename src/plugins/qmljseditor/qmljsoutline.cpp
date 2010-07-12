#include "qmljsoutline.h"
#include "qmloutlinemodel.h"

#include <coreplugin/ifile.h>
#include <QtGui/QVBoxLayout>

#include <QDebug>
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
    m_treeView(new QmlJSOutlineTreeView(this)),
    m_enableCursorSync(true),
    m_blockCursorSync(false)
{
    QVBoxLayout *layout = new QVBoxLayout;

    layout->setMargin(0);
    layout->setSpacing(0);
    layout->addWidget(m_treeView);

    setLayout(layout);
}

void QmlJSOutlineWidget::setEditor(QmlJSTextEditor *editor)
{
    m_editor = editor;

    m_treeView->setModel(m_editor.data()->outlineModel());
    connect(m_treeView->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            this, SLOT(updateSelectionInText(QItemSelection)));

    connect(m_editor.data(), SIGNAL(outlineModelIndexChanged(QModelIndex)),
            this, SLOT(updateSelectionInTree(QModelIndex)));
    connect(m_editor.data()->outlineModel(), SIGNAL(updated()),
            this, SLOT(modelUpdated()));
}

void QmlJSOutlineWidget::setCursorSynchronization(bool syncWithCursor)
{
    m_enableCursorSync = syncWithCursor;
    if (m_enableCursorSync)
        updateSelectionInTree(m_editor.data()->outlineModelIndex());
}

void QmlJSOutlineWidget::modelUpdated()
{
    m_treeView->expandAll();
}

void QmlJSOutlineWidget::updateSelectionInTree(const QModelIndex &index)
{
    if (!syncCursor())
        return;

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
