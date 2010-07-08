#include "cppoutline.h"

#include <TranslationUnit.h>
#include <Symbol.h>

#include <coreplugin/ifile.h>
#include <cplusplus/OverviewModel.h>

#include <QtCore/QDebug>
#include <QtGui/QVBoxLayout>
#include <QtCore/QTimer>

using namespace CppEditor::Internal;

enum {
    debug = false
};

CppOutlineTreeView::CppOutlineTreeView(QWidget *parent) :
    QTreeView(parent)
{
    // see also QmlJSOutlineTreeView
    setFocusPolicy(Qt::NoFocus);
    setFrameStyle(QFrame::NoFrame);
    setAttribute(Qt::WA_MacShowFocusRect, false);
    setUniformRowHeights(true);
    setHeaderHidden(true);
    setTextElideMode(Qt::ElideNone);
    setIndentation(20);
    setExpandsOnDoubleClick(false);
}

CppOutlineFilterModel::CppOutlineFilterModel(CPlusPlus::OverviewModel *sourceModel, QObject *parent) :
    QSortFilterProxyModel(parent),
    m_sourceModel(sourceModel)
{
    setSourceModel(m_sourceModel);
}

bool CppOutlineFilterModel::filterAcceptsRow(int sourceRow,
                                             const QModelIndex &sourceParent) const
{
    // ignore artifical "<Select Symbol>" entry
    if (!sourceParent.isValid() && sourceRow == 0) {
        return false;
    }
    // ignore generated symbols, e.g. by macro expansion (Q_OBJECT)
    const QModelIndex sourceIndex = m_sourceModel->index(sourceRow, 0, sourceParent);
    CPlusPlus::Symbol *symbol = m_sourceModel->symbolFromIndex(sourceIndex);
    if (symbol && symbol->isGenerated())
        return false;

    return QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent);
}


CppOutlineWidget::CppOutlineWidget(CPPEditor *editor) :
    TextEditor::IOutlineWidget(),
    m_editor(editor),
    m_treeView(new CppOutlineTreeView(this)),
    m_model(m_editor->overviewModel()),
    m_proxyModel(new CppOutlineFilterModel(m_model, this)),
    m_enableCursorSync(true),
    m_blockCursorSync(false)
{
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setMargin(0);
    layout->setSpacing(0);
    layout->addWidget(m_treeView);
    setLayout(layout);

    m_treeView->setModel(m_proxyModel);

    connect(m_model, SIGNAL(modelReset()), this, SLOT(modelUpdated()));
    modelUpdated();

    connect(m_editor, SIGNAL(overviewModelIndexChanged(QModelIndex)),
            this, SLOT(updateSelectionInTree(QModelIndex)));
    connect(m_treeView->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            this, SLOT(updateSelectionInText(QItemSelection)));
}

void CppOutlineWidget::setCursorSynchronization(bool syncWithCursor)
{
    m_enableCursorSync = syncWithCursor;
    if (m_enableCursorSync)
        updateSelectionInTree(m_editor->overviewModelIndex());
}

void CppOutlineWidget::modelUpdated()
{
    m_treeView->expandAll();
    updateSelectionInTree(m_editor->overviewModelIndex());
}

void CppOutlineWidget::updateSelectionInTree(const QModelIndex &index)
{
    if (!syncCursor())
        return;

    QModelIndex proxyIndex = m_proxyModel->mapFromSource(index);

    m_blockCursorSync = true;
    if (debug)
        qDebug() << "CppOutline - updating selection due to cursor move";

    m_treeView->selectionModel()->select(proxyIndex, QItemSelectionModel::ClearAndSelect);
    m_blockCursorSync = false;
}

void CppOutlineWidget::updateSelectionInText(const QItemSelection &selection)
{
    if (!syncCursor())
        return;

    if (!selection.indexes().isEmpty()) {
        QModelIndex proxyIndex = selection.indexes().first();
        QModelIndex index = m_proxyModel->mapToSource(proxyIndex);
        CPlusPlus::Symbol *symbol = m_model->symbolFromIndex(index);
        if (symbol) {
            m_blockCursorSync = true;
            unsigned line, column;

            m_model->document()->translationUnit()->getPosition(symbol->startOffset(), &line, &column);

            if (debug)
                qDebug() << "CppOutline - moving cursor to" << line << column - 1;

            // line has to be 1 based, column 0 based!
            m_editor->gotoLine(line, column - 1);
            m_blockCursorSync = false;
        }
    }
}

bool CppOutlineWidget::syncCursor()
{
    return m_enableCursorSync && !m_blockCursorSync;
}

bool CppOutlineWidgetFactory::supportsEditor(Core::IEditor *editor) const
{
    if (qobject_cast<CPPEditorEditable*>(editor))
        return true;
    return false;
}

TextEditor::IOutlineWidget *CppOutlineWidgetFactory::createWidget(Core::IEditor *editor)
{
    CPPEditorEditable *cppEditable = qobject_cast<CPPEditorEditable*>(editor);
    CPPEditor *cppEditor = qobject_cast<CPPEditor*>(cppEditable->widget());
    Q_ASSERT(cppEditor);

    CppOutlineWidget *widget = new CppOutlineWidget(cppEditor);

    return widget;
}
