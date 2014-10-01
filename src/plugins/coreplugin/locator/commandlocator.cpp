/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
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
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "commandlocator.h"

#include <coreplugin/actionmanager/command.h>

#include <utils/qtcassert.h>

#include <QAction>

namespace Core {

struct CommandLocatorPrivate
{
    QList<Core::Command *> commands;
};

CommandLocator::CommandLocator(Core::Id id,
                               const QString &displayName,
                               const QString &shortCutString,
                               QObject *parent) :
    Core::ILocatorFilter(parent),
    d(new CommandLocatorPrivate)
{
    setId(id);
    setDisplayName(displayName);
    setShortcutString(shortCutString);
}

CommandLocator::~CommandLocator()
{
    delete d;
}

void CommandLocator::appendCommand(Core::Command *cmd)
{
    d->commands.push_back(cmd);
}

QList<Core::LocatorFilterEntry> CommandLocator::matchesFor(QFutureInterface<Core::LocatorFilterEntry> &future, const QString &entry)
{
    QList<LocatorFilterEntry> goodEntries;
    QList<LocatorFilterEntry> betterEntries;
    // Get active, enabled actions matching text, store in list.
    // Reference via index in extraInfo.
    const QChar ampersand = QLatin1Char('&');
    const Qt::CaseSensitivity caseSensitivity_ = caseSensitivity(entry);
    const int count = d->commands.size();
    for (int i = 0; i < count; i++) {
        if (future.isCanceled())
            break;
        if (d->commands.at(i)->isActive()) {
            if (QAction *action = d->commands.at(i)->action())
                if (action->isEnabled()) {
                QString text = action->text();
                text.remove(ampersand);
                if (text.startsWith(entry, caseSensitivity_))
                    betterEntries.append(LocatorFilterEntry(this, text, QVariant(i)));
                else if (text.contains(entry, caseSensitivity_))
                    goodEntries.append(LocatorFilterEntry(this, text, QVariant(i)));
            }
        }
    }
    betterEntries.append(goodEntries);
    return betterEntries;
}

void CommandLocator::accept(Core::LocatorFilterEntry entry) const
{
    // Retrieve action via index.
    const int index = entry.internalData.toInt();
    QTC_ASSERT(index >= 0 && index < d->commands.size(), return);
    QAction *action = d->commands.at(index)->action();
    QTC_ASSERT(action->isEnabled(), return);
    action->trigger();
}

void CommandLocator::refresh(QFutureInterface<void> &)
{
}

}  // namespace Core
