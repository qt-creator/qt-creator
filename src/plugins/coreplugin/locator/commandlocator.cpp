// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "commandlocator.h"

#include <coreplugin/actionmanager/command.h>

#include <utils/qtcassert.h>
#include <utils/stringutils.h>

#include <QAction>

using namespace Utils;

namespace Core {

struct CommandLocatorPrivate
{
    QList<Command *> commands;
    QList<QPair<int, QString>> commandsData;
};

/*!
    \class Core::CommandLocator
    \inmodule QtCreator
    \internal
*/

CommandLocator::CommandLocator(Id id,
                               const QString &displayName,
                               const QString &shortCutString,
                               QObject *parent) :
    ILocatorFilter(parent),
    d(new CommandLocatorPrivate)
{
    setId(id);
    setDisplayName(displayName);
    setDefaultShortcutString(shortCutString);
}

CommandLocator::~CommandLocator()
{
    delete d;
}

void CommandLocator::appendCommand(Command *cmd)
{
    d->commands.push_back(cmd);
}

void CommandLocator::prepareSearch(const QString &entry)
{
    Q_UNUSED(entry)
    d->commandsData = {};
    const int count = d->commands.size();
    // Get active, enabled actions matching text, store in list.
    // Reference via index in extraInfo.
    for (int i = 0; i < count; ++i) {
        Command *command = d->commands.at(i);
        if (!command->isActive())
            continue;
        QAction *action = command->action();
        if (action && action->isEnabled())
            d->commandsData.append({i, action->text()});
    }
}

QList<LocatorFilterEntry> CommandLocator::matchesFor(QFutureInterface<LocatorFilterEntry> &future, const QString &entry)
{
    QList<LocatorFilterEntry> goodEntries;
    QList<LocatorFilterEntry> betterEntries;
    const Qt::CaseSensitivity entryCaseSensitivity = caseSensitivity(entry);
    for (const auto &pair : std::as_const(d->commandsData)) {
        if (future.isCanceled())
            break;

        const QString text = Utils::stripAccelerator(pair.second);
        const int index = text.indexOf(entry, 0, entryCaseSensitivity);
        if (index >= 0) {
            LocatorFilterEntry filterEntry(this, text, QVariant(pair.first));
            filterEntry.highlightInfo = {index, int(entry.length())};

            if (index == 0)
                betterEntries.append(filterEntry);
            else
                goodEntries.append(filterEntry);
        }
    }
    betterEntries.append(goodEntries);
    return betterEntries;
}

void CommandLocator::accept(const LocatorFilterEntry &entry,
                            QString *newText, int *selectionStart, int *selectionLength) const
{
    Q_UNUSED(newText)
    Q_UNUSED(selectionStart)
    Q_UNUSED(selectionLength)
    // Retrieve action via index.
    const int index = entry.internalData.toInt();
    QTC_ASSERT(index >= 0 && index < d->commands.size(), return);
    QAction *action = d->commands.at(index)->action();
    // avoid nested stack trace and blocking locator by delayed triggering
    QMetaObject::invokeMethod(action, [action] {
        if (action->isEnabled())
            action->trigger();
    }, Qt::QueuedConnection);
}

}  // namespace Core
