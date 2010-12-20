#ifndef CPPOUTLINE_H
#define CPPOUTLINE_H

#include "cppeditor.h"

#include <utils/navigationtreeview.h>
#include <texteditor/ioutlinewidget.h>

#include <QtGui/QSortFilterProxyModel>
#include <QtGui/QTreeView>

namespace CppEditor {
namespace Internal {

class CppOutlineTreeView : public Utils::NavigationTreeView
{
    Q_OBJECT
public:
    CppOutlineTreeView(QWidget *parent);

    void contextMenuEvent(QContextMenuEvent *event);
};

class CppOutlineFilterModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    CppOutlineFilterModel(CPlusPlus::OverviewModel *sourceModel, QObject *parent);
    // QSortFilterProxyModel
    bool filterAcceptsRow(int sourceRow,
                          const QModelIndex &sourceParent) const;
private:
    CPlusPlus::OverviewModel *m_sourceModel;
};

class CppOutlineWidget : public TextEditor::IOutlineWidget
{
    Q_OBJECT
public:
    CppOutlineWidget(CPPEditor *editor);

    // IOutlineWidget
    virtual QList<QAction*> filterMenuActions() const;
    virtual void setCursorSynchronization(bool syncWithCursor);

private slots:
    void modelUpdated();
    void updateSelectionInTree(const QModelIndex &index);
    void updateSelectionInText(const QItemSelection &selection);
    void updateTextCursor(const QModelIndex &index);

private:
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
