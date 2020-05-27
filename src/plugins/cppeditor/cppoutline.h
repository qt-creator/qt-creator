/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "cppeditorwidget.h"

#include <texteditor/ioutlinewidget.h>

#include <cpptools/abstractoverviewmodel.h>
#include <utils/navigationtreeview.h>

#include <QSortFilterProxyModel>

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
    CppOutlineFilterModel(CppTools::AbstractOverviewModel &sourceModel, QObject *parent);
    // QSortFilterProxyModel
    bool filterAcceptsRow(int sourceRow,
                          const QModelIndex &sourceParent) const override;
    Qt::DropActions supportedDragActions() const override;
private:
    CppTools::AbstractOverviewModel &m_sourceModel;
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
    void updateSelectionInTree(const QModelIndex &index);
    void updateTextCursor(const QModelIndex &index);
    void onItemActivated(const QModelIndex &index);
    bool syncCursor();

private:
    CppEditorWidget *m_editor;
    CppOutlineTreeView *m_treeView;
    QSortFilterProxyModel *m_proxyModel;

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
