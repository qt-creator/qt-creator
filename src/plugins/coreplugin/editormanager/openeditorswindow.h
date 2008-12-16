/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

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
