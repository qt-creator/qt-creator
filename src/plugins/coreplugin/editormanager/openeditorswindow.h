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

#include "documentmodel.h"
#include "editorview.h"

#include <QFrame>
#include <QIcon>
#include <QList>
#include <QTreeWidget>

QT_BEGIN_NAMESPACE
class QTreeWidgetItem;
QT_END_NAMESPACE

namespace Core {

class IEditor;

namespace Internal {

class OpenEditorsTreeWidget : public QTreeWidget {
public:
    explicit OpenEditorsTreeWidget(QWidget *parent = 0) : QTreeWidget(parent) {}
    ~OpenEditorsTreeWidget() {}
    QSize sizeHint() const;
};


class OpenEditorsWindow : public QFrame
{
    Q_OBJECT

public:
    enum Mode {ListMode, HistoryMode };

    explicit OpenEditorsWindow(QWidget *parent = 0);

    void setEditors(const QList<EditLocation> &globalHistory, EditorView *view);

    bool eventFilter(QObject *src, QEvent *e);
    void focusInEvent(QFocusEvent *);
    void setVisible(bool visible);
    void selectNextEditor();
    void selectPreviousEditor();
    QSize sizeHint() const;

public slots:
    void selectAndHide();

private:
    void editorClicked(QTreeWidgetItem *item);
    void selectEditor(QTreeWidgetItem *item);

    void addHistoryItems(const QList<EditLocation> &history, EditorView *view,
                         QSet<const DocumentModel::Entry *> &entriesDone);
    void addRemainingItems(EditorView *view,
                           QSet<const DocumentModel::Entry *> &entriesDone);
    void addItem(DocumentModel::Entry *entry, QSet<const DocumentModel::Entry *> &entriesDone,
                 EditorView *view);
    void ensureCurrentVisible();
    void selectUpDown(bool up);

    bool isSameFile(IEditor *editorA, IEditor *editorB) const;

    const QIcon m_emptyIcon;
    OpenEditorsTreeWidget *m_editorList;
};

} // namespace Internal
} // namespace Core
