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

#include "qmljseditor.h"

#include <texteditor/ioutlinewidget.h>

#include <QSortFilterProxyModel>

QT_BEGIN_NAMESPACE
class QAction;
QT_END_NAMESPACE

namespace Core { class IEditor; }

namespace QmlJS { class Editor; }

namespace QmlJSEditor {
namespace Internal {

class QmlJSOutlineTreeView;

class QmlJSOutlineFilterModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    QmlJSOutlineFilterModel(QObject *parent);
    // QSortFilterProxyModel
    bool filterAcceptsRow(int sourceRow,
                          const QModelIndex &sourceParent) const;
    QVariant data(const QModelIndex &index, int role) const;
    Qt::DropActions supportedDragActions() const;

    bool filterBindings() const;
    void setFilterBindings(bool filterBindings);
private:
    bool m_filterBindings;
};

class QmlJSOutlineWidget : public TextEditor::IOutlineWidget
{
    Q_OBJECT
public:
    QmlJSOutlineWidget(QWidget *parent = 0);

    void setEditor(QmlJSEditorWidget *editor);

    // IOutlineWidget
    virtual QList<QAction*> filterMenuActions() const override;
    virtual void setCursorSynchronization(bool syncWithCursor) override;
    virtual void restoreSettings(const QVariantMap &map) override;
    virtual QVariantMap settings() const override;

private:
    void updateSelectionInTree(const QModelIndex &index);
    void updateSelectionInText(const QItemSelection &selection);
    void updateTextCursor(const QModelIndex &index);
    void focusEditor();
    void setShowBindings(bool showBindings);
    bool syncCursor();

private:
    QmlJSOutlineTreeView *m_treeView = nullptr;
    QmlJSOutlineFilterModel *m_filterModel = nullptr;
    QmlJSEditorWidget *m_editor = nullptr;

    QAction *m_showBindingsAction = nullptr;

    bool m_enableCursorSync = true;
    bool m_blockCursorSync = false;
};

class QmlJSOutlineWidgetFactory : public TextEditor::IOutlineWidgetFactory
{
    Q_OBJECT
public:
    bool supportsEditor(Core::IEditor *editor) const;
    TextEditor::IOutlineWidget *createWidget(Core::IEditor *editor);
};

} // namespace Internal
} // namespace QmlJSEditor
