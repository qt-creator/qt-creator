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
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/
***************************************************************************/

#ifndef ACTIONMANAGERINTERFACE_H
#define ACTIONMANAGERINTERFACE_H

#include "coreplugin/core_global.h"

#include <coreplugin/actionmanager/iactioncontainer.h>
#include <coreplugin/actionmanager/icommand.h>

#include <QtCore/QObject>
#include <QtCore/QList>

QT_BEGIN_NAMESPACE
class QAction;
class QShortcut;
class QString;
QT_END_NAMESPACE

namespace Core {

class CORE_EXPORT ActionManagerInterface : public QObject
{
    Q_OBJECT
public:
    ActionManagerInterface(QObject *parent = 0) : QObject(parent) {}
    virtual ~ActionManagerInterface() {}

    virtual IActionContainer *createMenu(const QString &id) = 0;
    virtual IActionContainer *createMenuBar(const QString &id) = 0;

    virtual ICommand *registerAction(QAction *action, const QString &id, const QList<int> &context) = 0;
    virtual ICommand *registerShortcut(QShortcut *shortcut, const QString &id, const QList<int> &context) = 0;

    virtual ICommand *registerAction(QAction *action, const QString &id) = 0;

    virtual void addAction(ICommand *action, const QString &globalGroup) = 0;
    virtual void addMenu(IActionContainer *menu, const QString &globalGroup) = 0;

    virtual ICommand *command(const QString &id) const = 0;
    virtual IActionContainer *actionContainer(const QString &id) const = 0;
};

} // namespace Core

#endif // ACTIONMANAGERINTERFACE_H
