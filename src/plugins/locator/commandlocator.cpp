/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
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

#include "commandlocator.h"

#include <coreplugin/actionmanager/command.h>
#include <coreplugin/icore.h>

#include <utils/qtcassert.h>

#include <QAction>

namespace Locator {

struct CommandLocatorPrivate {
    CommandLocatorPrivate(const QString &prefix,
                          const QString &displayName) :
        m_prefix(prefix), m_displayName(displayName) {}

    const QString m_prefix;
    const QString m_displayName;

    QList<Core::Command *> commands;
};

CommandLocator::CommandLocator(const QString &prefix,
                               const QString &displayName,
                               const QString &shortCutString,
                               QObject *parent) :
    Locator::ILocatorFilter(parent),
    d(new CommandLocatorPrivate(prefix, displayName))
{
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

QString CommandLocator::displayName() const
{
    return d->m_displayName;
}

QString CommandLocator::id() const
{
    return d->m_prefix;
}

ILocatorFilter::Priority CommandLocator::priority() const
{
    return Medium;
}

QList<Locator::FilterEntry> CommandLocator::matchesFor(QFutureInterface<Locator::FilterEntry> &future, const QString &entry)
{
    QList<FilterEntry> filters;
    // Get active, enabled actions matching text, store in list.
    // Reference via index in extraInfo.
    const QChar ampersand = QLatin1Char('&');
    const int count = d->commands.size();
    for (int i = 0; i  < count; i++) {
        if (future.isCanceled())
            break;
        if (d->commands.at(i)->isActive()) {
            if (QAction *action = d->commands.at(i)->action())
                if (action->isEnabled()) {
                QString text = action->text();
                text.remove(ampersand);
                if (text.contains(entry, Qt::CaseInsensitive))
                    filters.append(FilterEntry(this, text, QVariant(i)));
            }
        }
    }
    return filters;
}

void CommandLocator::accept(Locator::FilterEntry entry) const
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

}  // namespace Locator
