#include "cppoutline.h"

#include <TranslationUnit.h>
#include <Symbol.h>

#include <coreplugin/ifile.h>
#include <cplusplus/OverviewModel.h>

#include <QtGui/QVBoxLayout>
#include <QtCore/QDebug>

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

CppOutlineFilterModel::CppOutlineFilterModel(QObject *parent) :
    QSortFilterProxyModel(parent)
{
}

bool CppOutlineFilterModel::filterAcceptsRow(int sourceRow,
                                             const QModelIndex &sourceParent) const
{
    // ignore artifical "<Select Symbol>" entry
    if (!sourceParent.isValid() && sourceRow == 0) {
        return false;
    }
    return QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent);
}


CppOutlineWidget::CppOutlineWidget(CPPEditor *editor) :
    TextEditor::IOutlineWidget(),
    m_editor(editor),
    m_treeView(new CppOutlineTreeView(this)),
    m_model(new CPlusPlus::OverviewModel(this)),
    m_proxyModel(new CppOutlineFilterModel(this)),
    m_enableCursorSync(true),
    m_blockCursorSync(false)
{
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setMargin(0);
    layout->setSpacing(0);
    layout->addWidget(m_treeView);
    setLayout(layout);

    m_proxyModel->setSourceModel(m_model);
    m_treeView->setModel(m_proxyModel);

    CppTools::CppModelManagerInterface *modelManager = CppTools::CppModelManagerInterface::instance();

    connect(modelManager, SIGNAL(documentUpdated(CPlusPlus::Document::Ptr)),
            this, SLOT(updateOutline(CPlusPlus::Document::Ptr)));

    if (modelManager->snapshot().contains(editor->file()->fileName())) {
        updateOutline(modelManager->snapshot().document(editor->file()->fileName()));
    }

    connect(m_editor, SIGNAL(cursorPositionChanged()),
            this, SLOT(updateSelectionInTree()));
    connect(m_treeView->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            this, SLOT(updateSelectionInText(QItemSelection)));
}

void CppOutlineWidget::setCursorSynchronization(bool syncWithCursor)
{
    m_enableCursorSync = syncWithCursor;
    if (m_enableCursorSync)
        updateSelectionInTree();
}

void CppOutlineWidget::updateOutline(CPlusPlus::Document::Ptr document)
{
    m_document = document;
    if (document && m_editor
            && (document->fileName() == m_editor->file()->fileName())
            && (document->editorRevision() == m_editor->editorRevision())) {
        if (debug)
            qDebug() << "CppOutline - rebuilding model";
        m_model->rebuild(document);
        m_treeView->expandAll();
        updateSelectionInTree();
    }
}

void CppOutlineWidget::updateSelectionInTree()
{
    if (!syncCursor())
        return;

    int line = m_editor->textCursor().blockNumber();
    int column = m_editor->textCursor().columnNumber();

    QModelIndex index = indexForPosition(QModelIndex(), line, column);
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
            m_document->translationUnit()->getPosition(symbol->startOffset(), &line, &column);

            if (debug)
                qDebug() << "CppOutline - moving cursor to" << line << column - 1;

            // line has to be 1 based, column 0 based!
            m_editor->gotoLine(line, column - 1);
            m_blockCursorSync = false;
        }
    }
}

QModelIndex CppOutlineWidget::indexForPosition(const QModelIndex &rootIndex, int line, int column)
{
    QModelIndex result = rootIndex;

    const int rowCount = m_model->rowCount(rootIndex);
    for (int row = 0; row < rowCount; ++row) {
        QModelIndex index = m_model->index(row, 0, rootIndex);
        CPlusPlus::Symbol *symbol = m_model->symbolFromIndex(index);
        if (symbol && positionInsideSymbol(line, column, symbol)) {
            // recurse to children
            result = indexForPosition(index, line, column);
        }
    }

    return result;
}

bool CppOutlineWidget::positionInsideSymbol(unsigned cursorLine, unsigned cursorColumn, CPlusPlus::Symbol *symbol) const
{
    if (!m_document)
        return false;
    CPlusPlus::TranslationUnit *translationUnit = m_document->translationUnit();

    unsigned symbolStartLine = -1;
    unsigned symbolStartColumn = -1;

    translationUnit->getPosition(symbol->startOffset(), &symbolStartLine, &symbolStartColumn);

    // normalize to 0 based
    --symbolStartLine;
    --symbolStartColumn;

    if (symbolStartLine < cursorLine
            || (symbolStartLine == cursorLine && symbolStartColumn <= cursorColumn)) {
        unsigned symbolEndLine = -1;
        unsigned symbolEndColumn = -1;
        translationUnit->getPosition(symbol->endOffset(), &symbolEndLine, &symbolEndColumn);

        // normalize to 0 based
        --symbolEndLine;
        --symbolEndColumn;

        if (symbolEndLine > cursorLine
                || (symbolEndLine == cursorLine && symbolEndColumn >= cursorColumn)) {
            return true;
        }
    }
    return false;
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
