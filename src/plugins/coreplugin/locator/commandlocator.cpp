/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "commandlocator.h"

#include <coreplugin/actionmanager/command.h>

#include <utils/qtcassert.h>

#include <QAction>

namespace Core {

struct CommandLocatorPrivate
{
    QList<Command *> commands;
};

CommandLocator::CommandLocator(Id id,
                               const QString &displayName,
                               const QString &shortCutString,
                               QObject *parent) :
    ILocatorFilter(parent),
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

void CommandLocator::appendCommand(Command *cmd)
{
    d->commands.push_back(cmd);
}

QList<LocatorFilterEntry> CommandLocator::matchesFor(QFutureInterface<LocatorFilterEntry> &future, const QString &entry)
{
    QList<LocatorFilterEntry> goodEntries;
    QList<LocatorFilterEntry> betterEntries;
    // Get active, enabled actions matching text, store in list.
    // Reference via index in extraInfo.
    const QChar ampersand = QLatin1Char('&');
    const Qt::CaseSensitivity entryCaseSensitivity = caseSensitivity(entry);
    const int count = d->commands.size();
    for (int i = 0; i < count; i++) {
        if (future.isCanceled())
            break;
        if (!d->commands.at(i)->isActive())
            continue;

        QAction *action = d->commands.at(i)->action();
        if (action && action->isEnabled()) {
            QString text = action->text();
            text.remove(ampersand);
            const int index = text.indexOf(entry, 0, entryCaseSensitivity);
            if (index >= 0) {
                LocatorFilterEntry filterEntry(this, text, QVariant(i));
                filterEntry.highlightInfo = {index, entry.length()};

                if (index == 0)
                    betterEntries.append(filterEntry);
                else
                    goodEntries.append(filterEntry);
            }
        }
    }
    betterEntries.append(goodEntries);
    return betterEntries;
}

void CommandLocator::accept(LocatorFilterEntry entry) const
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
