#ifndef CPPOUTLINE_H
#define CPPOUTLINE_H

#include "cppeditor.h"

#include <texteditor/ioutlinewidget.h>

#include <QtGui/QSortFilterProxyModel>
#include <QtGui/QTreeView>

namespace CppEditor {
namespace Internal {

class CppOutlineTreeView : public QTreeView
{
    Q_OBJECT
public:
    CppOutlineTreeView(QWidget *parent);
};

class CppOutlineFilterModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    CppOutlineFilterModel(QObject *parent);
    // QSortFilterProxyModel
    bool filterAcceptsRow(int sourceRow,
                          const QModelIndex &sourceParent) const;
};

class CppOutlineWidget : public TextEditor::IOutlineWidget
{
    Q_OBJECT
public:
    CppOutlineWidget(CPPEditor *editor);

    // IOutlineWidget
    virtual void setCursorSynchronization(bool syncWithCursor);

private slots:
    void modelUpdated();
    void updateSelectionInTree();
    void updateSelectionInText(const QItemSelection &selection);

private:
    QModelIndex indexForPosition(const QModelIndex &rootIndex, int line, int column);
    bool positionInsideSymbol(unsigned cursorLine, unsigned cursorColumn, CPlusPlus::Symbol *symbol) const;
    bool syncCursor();

private:
    CPPEditor *m_editor;
    CppOutlineTreeView *m_treeView;
    CPlusPlus::OverviewModel *m_model;
    CppOutlineFilterModel *m_proxyModel;

    bool m_enableCursorSync;
    bool m_blockCursorSync;
};

class CppOutlineWidgetFactory : public TextEditor::IOutlineWidgetFactory
{
    Q_OBJECT
public:
    bool supportsEditor(Core::IEditor *editor) const;
    TextEditor::IOutlineWidget *createWidget(Core::IEditor *editor);
};

} // namespace Internal
} // namespace CppEditor

#endif // CPPOUTLINE_H
