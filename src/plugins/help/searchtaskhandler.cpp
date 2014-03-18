/****************************************************************************
**
** Copyright (C) 2014 Orgad Shaneh <orgads@gmail.com>.
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

#include "searchtaskhandler.h"

#include <projectexplorer/task.h>

#include <QAction>
#include <QApplication>
#include <QClipboard>

using namespace Help::Internal;

bool SearchTaskHandler::canHandle(const ProjectExplorer::Task &task) const
{
    return !task.description.isEmpty()
            && !task.description.startsWith(QLatin1Char('\n'));
}

void SearchTaskHandler::handle(const ProjectExplorer::Task &task)
{
    const int eol = task.description.indexOf(QLatin1Char('\n'));
    const QUrl url(QLatin1String("https://www.google.com/search?q=") + task.description.left(eol));
    emit search(url);
}

QAction *SearchTaskHandler::createAction(QObject *parent) const
{
    return new QAction(tr("Get Help Online"), parent);
}
