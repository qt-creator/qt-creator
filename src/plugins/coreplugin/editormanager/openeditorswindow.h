/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
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

namespace Internal {

class OpenEditorsWindow : public QWidget
{
    Q_OBJECT

public:
    enum Mode {ListMode, HistoryMode };

    OpenEditorsWindow(QWidget *parent = 0);
    ~OpenEditorsWindow() {}

    void setMode(Mode mode);
    Mode mode() const { return m_mode; }

    bool event(QEvent *e);
    bool eventFilter(QObject *src, QEvent *e);
    void focusInEvent(QFocusEvent *);
    void setVisible(bool visible);
    void setSelectedEditor(IEditor *editor);
    void selectNextEditor();
    void selectPreviousEditor();
    IEditor *selectedEditor() const { return m_current; }

private slots:
    void updateEditorList(IEditor *current = 0);
    void editorClicked(QTreeWidgetItem *item);
    void selectEditor(QTreeWidgetItem *item);
    void selectAndHide();

private:
    static const int WIDTH;
    static const int HEIGHT;
    static const int MARGIN;

    static void updateItem(QTreeWidgetItem *item, IEditor *editor);
    void updateList();
    void updateHistory();
    void updateSelectedEditor();
    bool isCentering();
    void centerOnItem(int selectedIndex);
    void selectUpDown(bool up);

    QTreeWidget *m_editorList;
    Mode m_mode;
    QTimer m_autoHide;
    IEditor *m_current;
};

} // namespace Internal
} // namespace Core

#endif // OPENEDITORSWINDOW_H
