/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef OPENEDITORSWINDOW_H
#define OPENEDITORSWINDOW_H

#include "editorview.h"

#include <QFrame>
#include <QIcon>
#include <QList>

QT_BEGIN_NAMESPACE
class QTreeWidgetItem;
class QTreeWidget;
QT_END_NAMESPACE

namespace Core {

class IDocument;
class IEditor;
class OpenEditorsModel;

namespace Internal {

class EditorHistoryItem;

class OpenEditorsWindow : public QFrame
{
    Q_OBJECT

public:
    enum Mode {ListMode, HistoryMode };

    explicit OpenEditorsWindow(QWidget *parent = 0);

    void setEditors(const QList<EditLocation> &globalHistory, EditorView *view, OpenEditorsModel *model);

    bool eventFilter(QObject *src, QEvent *e);
    void focusInEvent(QFocusEvent *);
    void setVisible(bool visible);
    void selectNextEditor();
    void selectPreviousEditor();

public slots:
    void selectAndHide();

private slots:
    void editorClicked(QTreeWidgetItem *item);
    void selectEditor(QTreeWidgetItem *item);

private:
    static void updateItem(QTreeWidgetItem *item, IEditor *editor);
    void addHistoryItems(const QList<EditLocation> &history, EditorView *view,
                         OpenEditorsModel *model, QSet<IDocument*> &documentsDone);
    void ensureCurrentVisible();
    bool isCentering();
    void centerOnItem(int selectedIndex);
    void selectUpDown(bool up);

    bool isSameFile(IEditor *editorA, IEditor *editorB) const;

    const QIcon m_emptyIcon;
    QTreeWidget *m_editorList;
};

} // namespace Internal
} // namespace Core

#endif // OPENEDITORSWINDOW_H
