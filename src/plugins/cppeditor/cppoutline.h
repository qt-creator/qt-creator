// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cppeditorwidget.h"
#include "cppoutlinemodel.h"

#include <texteditor/ioutlinewidget.h>

#include <utils/navigationtreeview.h>

#include <QSortFilterProxyModel>
#include <QTimer>

namespace CppEditor {
namespace Internal {

class CppOutlineTreeView : public Utils::NavigationTreeView
{
    Q_OBJECT
public:
    CppOutlineTreeView(QWidget *parent);

    void contextMenuEvent(QContextMenuEvent *event) override;
};

class CppOutlineFilterModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    CppOutlineFilterModel(OutlineModel &sourceModel, QObject *parent);
    // QSortFilterProxyModel
    bool filterAcceptsRow(int sourceRow,
                          const QModelIndex &sourceParent) const override;
    Qt::DropActions supportedDragActions() const override;
private:
    OutlineModel &m_sourceModel;
};

class CppOutlineWidget : public TextEditor::IOutlineWidget
{
    Q_OBJECT
public:
    CppOutlineWidget(CppEditorWidget *editor);

    // IOutlineWidget
    QList<QAction*> filterMenuActions() const override;
    void setCursorSynchronization(bool syncWithCursor) override;
    bool isSorted() const override;
    void setSorted(bool sorted) override;

    void restoreSettings(const QVariantMap &map) override;
    QVariantMap settings() const override;
private:
    void modelUpdated();
    void updateIndex();
    void updateIndexNow();
    void updateTextCursor(const QModelIndex &index);
    void onItemActivated(const QModelIndex &index);
    bool syncCursor();

private:
    CppEditorWidget *m_editor;
    CppOutlineTreeView *m_treeView;
    OutlineModel * const m_model;
    QSortFilterProxyModel *m_proxyModel;
    QTimer m_updateIndexTimer;

    bool m_enableCursorSync;
    bool m_blockCursorSync;
    bool m_sorted;
};

class CppOutlineWidgetFactory : public TextEditor::IOutlineWidgetFactory
{
    Q_OBJECT
public:
    bool supportsEditor(Core::IEditor *editor) const override;
    bool supportsSorting() const override { return true; }
    TextEditor::IOutlineWidget *createWidget(Core::IEditor *editor) override;
};

} // namespace Internal
} // namespace CppEditor
