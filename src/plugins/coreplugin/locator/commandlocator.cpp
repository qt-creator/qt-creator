// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "commandlocator.h"

#include "../actionmanager/command.h"

#include <utils/stringutils.h>

#include <QAction>
#include <QPointer>

using namespace Utils;

namespace Core {

CommandLocator::CommandLocator(Id id, const QString &displayName, const QString &shortCutString,
                               QObject *parent)
    : ILocatorFilter(parent)
{
    setId(id);
    setDisplayName(displayName);
    setDefaultShortcutString(shortCutString);
}

LocatorMatcherTasks CommandLocator::matchers()
{
    using namespace Tasking;

    TreeStorage<LocatorStorage> storage;

    const auto onSetup = [storage, commands = m_commands] {
        const QString input = storage->input();
        const Qt::CaseSensitivity inputCaseSensitivity = caseSensitivity(input);
        LocatorFilterEntries goodEntries;
        LocatorFilterEntries betterEntries;
        for (Command *command : commands) {
            if (!command->isActive())
                continue;

            QAction *action = command->action();
            if (!action || !action->isEnabled())
                continue;

            const QString text = Utils::stripAccelerator(action->text());
            const int index = text.indexOf(input, 0, inputCaseSensitivity);
            if (index >= 0) {
                LocatorFilterEntry entry;
                entry.displayName = text;
                entry.acceptor = [actionPointer = QPointer(action)] {
                    if (actionPointer) {
                        QMetaObject::invokeMethod(actionPointer, [actionPointer] {
                            if (actionPointer && actionPointer->isEnabled())
                                actionPointer->trigger();
                        }, Qt::QueuedConnection);
                    }
                    return AcceptResult();
                };
                entry.highlightInfo = {index, int(input.length())};
                if (index == 0)
                    betterEntries.append(entry);
                else
                    goodEntries.append(entry);
            }
        }
        storage->reportOutput(betterEntries + goodEntries);
    };
    return {{Sync(onSetup), storage}};
}

}  // namespace Core
