/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef CPPOUTLINE_H
#define CPPOUTLINE_H

#include "cppeditor.h"

#include <texteditor/ioutlinewidget.h>

#include <utils/navigationtreeview.h>

#include <QSortFilterProxyModel>

namespace CPlusPlus { class OverviewModel; }

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
    Qt::DropActions supportedDragActions() const;
private:
    CPlusPlus::OverviewModel *m_sourceModel;
};

class CppOutlineWidget : public TextEditor::IOutlineWidget
{
    Q_OBJECT
public:
    CppOutlineWidget(CppEditorWidget *editor);

    // IOutlineWidget
    virtual QList<QAction*> filterMenuActions() const;
    virtual void setCursorSynchronization(bool syncWithCursor);

private slots:
    void modelUpdated();
    void updateSelectionInTree(const QModelIndex &index);
    void updateTextCursor(const QModelIndex &index);
    void onItemActivated(const QModelIndex &index);

private:
    bool syncCursor();

private:
    CppEditorWidget *m_editor;
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
