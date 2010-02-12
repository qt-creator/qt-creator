/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include "commandlocator.h"

#include <coreplugin/actionmanager/command.h>
#include <coreplugin/icore.h>

#include <utils/qtcassert.h>

#include <QtGui/QAction>

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

QList<Locator::FilterEntry> CommandLocator::matchesFor(const QString &entry)
{
    QList<FilterEntry> filters;
    // Get active, enabled actions matching text, store in list.
    // Reference via index in extraInfo.
    const QChar ampersand = QLatin1Char('&');
    const int count = d->commands.size();
    for (int i = 0; i  < count; i++) {
        if (d->commands.at(i)->isActive()) {
            if (QAction *action = d->commands.at(i)->action())
                if (action->isEnabled()) {
                QString text = action->text();
                text.remove(ampersand);
                if (text.contains(entry, Qt::CaseInsensitive)) {
                    filters.append(FilterEntry(this, text, QVariant(i)));
                }
            }
        }
    }
    return filters;
}

void CommandLocator::accept(Locator::FilterEntry entry) const
{
    // Retrieve action via index.
    const int index = entry.internalData.toInt();
    QTC_ASSERT(index >= 0 && index < d->commands.size(), return)
    QAction *action = d->commands.at(index)->action();
    QTC_ASSERT(action->isEnabled(), return)
    action->trigger();
}

void CommandLocator::setEnabled(bool e)
{
    setHidden(!e);
}

void CommandLocator::refresh(QFutureInterface<void> &)
{
}

}  // namespace Locator
