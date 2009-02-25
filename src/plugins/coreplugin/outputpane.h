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

#ifndef OUTPUTPANE_H
#define OUTPUTPANE_H

#include "core_global.h"

#include <QtCore/QMap>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE
class QAction;
class QComboBox;
class QToolButton;
class QStackedWidget;
class QPushButton;
QT_END_NAMESPACE

namespace Core {

class IMode;
class IOutputPane;

namespace Internal {
class OutputPaneManager;
class MainWindow;
}


class CORE_EXPORT OutputPanePlaceHolder : public QWidget
{
    friend class Core::Internal::OutputPaneManager; // needs to set m_visible and thus access m_current
    Q_OBJECT
public:
    OutputPanePlaceHolder(Core::IMode *mode, QWidget *parent = 0);
    ~OutputPanePlaceHolder();
    void setCloseable(bool b);
    bool closeable();
    static OutputPanePlaceHolder *getCurrent() { return m_current; }

private slots:
    void currentModeChanged(Core::IMode *);
private:
    Core::IMode *m_mode;
    bool m_closeable;
    static OutputPanePlaceHolder* m_current;
};

namespace Internal {

class OutputPaneManager : public QWidget
{
    Q_OBJECT

public:
    void init();
    static OutputPaneManager *instance();
    void setCloseable(bool b);
    bool closeable();
    QWidget *buttonsWidget();
    void updateStatusButtons(bool visible);

public slots:
    void slotHide();
    void shortcutTriggered();

protected:
    void focusInEvent(QFocusEvent *e);

private slots:
    void changePage();
    void showPage(bool focus);
    void togglePage(bool focus);
    void clearPage();
    void updateToolTip();
    void buttonTriggered();

private:
    // the only class that is allowed to create and destroy
    friend class MainWindow;

    static void create();
    static void destroy();

    OutputPaneManager(QWidget *parent = 0);
    ~OutputPaneManager();

    void showPage(int idx, bool focus);
    void ensurePageVisible(int idx);
    int findIndexForPage(IOutputPane *out);
    QComboBox *m_widgetComboBox;
    QToolButton *m_clearButton;
    QToolButton *m_closeButton;
    QAction *m_closeAction;

    QMap<int, Core::IOutputPane*> m_pageMap;
    int m_lastIndex;

    QStackedWidget *m_outputWidgetPane;
    QStackedWidget *m_opToolBarWidgets;
    QWidget *m_buttonsWidget;
    QMap<int, QPushButton *> m_buttons;
    QMap<QAction *, int> m_actions;
};

} // namespace Internal
} // namespace Core

#endif // OUTPUTPANE_H
