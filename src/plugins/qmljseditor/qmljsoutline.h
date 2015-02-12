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

#ifndef QMLJSOUTLINE_H
#define QMLJSOUTLINE_H

#include "qmljseditor.h"

#include <texteditor/ioutlinewidget.h>

#include <QSortFilterProxyModel>

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
    virtual QList<QAction*> filterMenuActions() const;
    virtual void setCursorSynchronization(bool syncWithCursor);
    virtual void restoreSettings(const QVariantMap &map);
    virtual QVariantMap settings() const;

private slots:
    void modelUpdated();
    void updateSelectionInTree(const QModelIndex &index);
    void updateSelectionInText(const QItemSelection &selection);
    void updateTextCursor(const QModelIndex &index);
    void focusEditor();
    void setShowBindings(bool showBindings);

private:
    bool syncCursor();

private:
    QmlJSOutlineTreeView *m_treeView;
    QmlJSOutlineFilterModel *m_filterModel;
    QmlJSEditorWidget *m_editor;

    QAction *m_showBindingsAction;

    bool m_enableCursorSync;
    bool m_blockCursorSync;
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

#endif // QMLJSOUTLINE_H
