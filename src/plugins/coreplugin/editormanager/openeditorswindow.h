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

#ifndef OPENEDITORSWINDOW_H
#define OPENEDITORSWINDOW_H

#include "editorview.h"

#include <QFrame>
#include <QIcon>
#include <QList>
#include <QTreeWidget>

QT_BEGIN_NAMESPACE
class QTreeWidgetItem;
QT_END_NAMESPACE

namespace Core {

class IDocument;
class IEditor;

namespace Internal {

class OpenEditorsTreeWidget : public QTreeWidget {
public:
    explicit OpenEditorsTreeWidget(QWidget *parent = 0) : QTreeWidget(parent) {}
    ~OpenEditorsTreeWidget() {}
    QSize sizeHint() const;
};

class EditorHistoryItem;

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

private slots:
    void editorClicked(QTreeWidgetItem *item);
    void selectEditor(QTreeWidgetItem *item);

private:
    void addHistoryItems(const QList<EditLocation> &history, EditorView *view, QSet<IDocument*> &documentsDone);
    void addRestoredItems();
    void ensureCurrentVisible();
    void selectUpDown(bool up);

    bool isSameFile(IEditor *editorA, IEditor *editorB) const;

    const QIcon m_emptyIcon;
    OpenEditorsTreeWidget *m_editorList;
};

} // namespace Internal
} // namespace Core

#endif // OPENEDITORSWINDOW_H
