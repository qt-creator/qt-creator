/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef OPENEDITORSWINDOW_H
#define OPENEDITORSWINDOW_H

#include <QtCore/QTimer>
#include <QtGui/QWidget>
#include <QtGui/QKeyEvent>
#include <QtGui/QFocusEvent>
#include <QtGui/QTreeWidget>
#include <QtDebug>

namespace Core {

class IEditor;
class OpenEditorsModel;

namespace Internal {

class EditorHistoryItem;
class EditorView;

class OpenEditorsWindow : public QWidget
{
    Q_OBJECT

public:
    enum Mode {ListMode, HistoryMode };

    OpenEditorsWindow(QWidget *parent = 0);
    ~OpenEditorsWindow() {}

    void setEditors(EditorView *mainView, EditorView *view, OpenEditorsModel *model);

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
    static const int WIDTH;
    static const int HEIGHT;
    static const int MARGIN;

    static void updateItem(QTreeWidgetItem *item, IEditor *editor);
    void ensureCurrentVisible();
    bool isCentering();
    void centerOnItem(int selectedIndex);
    void selectUpDown(bool up);

    bool isSameFile(IEditor *editorA, IEditor *editorB) const;

    QTreeWidget *m_editorList;
};

} // namespace Internal
} // namespace Core

#endif // OPENEDITORSWINDOW_H
