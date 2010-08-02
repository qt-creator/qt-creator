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
#include "helpconstants.h"

#include <coreplugin/icore.h>

#include <QtGui/QKeyEvent>

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
    installEventFilter(this);
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
