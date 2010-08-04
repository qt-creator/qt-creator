/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include "externalhelpwindow.h"

#include "centralwidget.h"
#include "helpconstants.h"
#include "openpagesmanager.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>

#include <QtGui/QAction>
#include <QtGui/QHBoxLayout>
#include <QtGui/QKeyEvent>
#include <QtGui/QStatusBar>
#include <QtGui/QToolButton>

using namespace Help::Internal;

ExternalHelpWindow::ExternalHelpWindow(QWidget *parent)
    : QMainWindow(parent)
{
    QSettings *settings = Core::ICore::instance()->settings();
    settings->beginGroup(Help::Constants::ID_MODE_HELP);

    const QVariant geometry = settings->value(QLatin1String("geometry"));
    if (geometry.isValid())
        restoreGeometry(geometry.toByteArray());
    else
        resize(640, 480);

    settings->endGroup();

    QAction *action = new QAction(this);
    action->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_I));
    connect(action, SIGNAL(triggered()), this, SIGNAL(activateIndex()));
    addAction(action);

    action = new QAction(this);
    action->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_C));
    connect(action, SIGNAL(triggered()), this, SIGNAL(activateContents()));
    addAction(action);

    action = new QAction(this);
    action->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_Slash));
    connect(action, SIGNAL(triggered()), this, SIGNAL(activateSearch()));
    addAction(action);

    action = new QAction(this);
    action->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_B));
    connect(action, SIGNAL(triggered()), this, SIGNAL(activateBookmarks()));
    addAction(action);

    action = new QAction(this);
    action->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_O));
    connect(action, SIGNAL(triggered()), this, SIGNAL(activateOpenPages()));
    addAction(action);

    action = new QAction(this);
    action->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Plus));
    connect(action, SIGNAL(triggered()), CentralWidget::instance(), SLOT(zoomIn()));
    addAction(action);

    action = new QAction(this);
    action->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Minus));
    connect(action, SIGNAL(triggered()), CentralWidget::instance(), SLOT(zoomOut()));
    addAction(action);

    action = new QAction(this);
    action->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_0));
    connect(action, SIGNAL(triggered()), CentralWidget::instance(), SLOT(resetZoom()));
    addAction(action);

    QAction *ctrlTab = new QAction(this);
    connect(ctrlTab, SIGNAL(triggered()), &OpenPagesManager::instance(),
        SLOT(gotoPreviousPage()));
    addAction(ctrlTab);

    QAction *ctrlShiftTab = new QAction(this);
    connect(ctrlShiftTab, SIGNAL(triggered()), &OpenPagesManager::instance(),
        SLOT(gotoNextPage()));
    addAction(ctrlShiftTab);

    action = new QAction(QIcon(Core::Constants::ICON_TOGGLE_SIDEBAR),
        tr("Show Sidebar"), this);
    connect(action, SIGNAL(triggered()), this, SIGNAL(showHideSidebar()));

#ifdef Q_WS_MAC
    action->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_0));
    ctrlTab->setShortcut(QKeySequence(tr("Alt+Tab")));
    ctrlShiftTab->setShortcut(QKeySequence(tr("Alt+Shift+Tab")));
#else
    action->setShortcut(QKeySequence(Qt::ALT + Qt::Key_0));
    ctrlTab->setShortcut(QKeySequence(tr("Ctrl+Tab")));
    ctrlShiftTab->setShortcut(QKeySequence(tr("Ctrl+Shift+Tab")));
#endif

    QToolButton *button = new QToolButton;
    button->setDefaultAction(action);

    QStatusBar *statusbar = statusBar();
    statusbar->show();
    statusbar->setProperty("p_styled", true);
    statusbar->addPermanentWidget(button);

    QWidget *w = new QWidget;
    QHBoxLayout *layout = new QHBoxLayout(w);
    layout->addStretch(1);
    statusbar->insertWidget(1, w, 1);

    installEventFilter(this);
    setWindowTitle(tr("Qt Creator Offline Help"));
}

ExternalHelpWindow::~ExternalHelpWindow()
{
}

void ExternalHelpWindow::closeEvent(QCloseEvent *event)
{
    QSettings *settings = Core::ICore::instance()->settings();
    settings->beginGroup(Help::Constants::ID_MODE_HELP);
    settings->setValue(QLatin1String("geometry"), saveGeometry());
    settings->endGroup();

    QMainWindow::closeEvent(event);
}

bool ExternalHelpWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == this) {
        if (event->type() == QEvent::KeyPress) {
            QKeyEvent *keyEvent = static_cast<QKeyEvent*> (event);
            switch (keyEvent->key()) {
                case Qt::Key_Escape:
                    Core::ICore::instance()->mainWindow()->activateWindow();
                default:
                    break;
            }
        }
    }
    return QMainWindow::eventFilter(obj, event);
}
