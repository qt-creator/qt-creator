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

#ifndef COMMANDLOCATOR_H
#define COMMANDLOCATOR_H

#include "locator_global.h"
#include "ilocatorfilter.h"
#include <QtCore/QList>

QT_BEGIN_NAMESPACE
class QAction;
QT_END_NAMESPACE

namespace Core {
    class Command;
}

namespace Locator {
/* Command locators: Provides completion for a set of
 * Core::Command's by sub-string of their action's text. */

struct CommandLocatorPrivate;

class LOCATOR_EXPORT CommandLocator : public Locator::ILocatorFilter
{
    Q_DISABLE_COPY(CommandLocator)
    Q_OBJECT
public:
    explicit CommandLocator(const QString &prefix,
                            const QString &displayName,
                            const QString &shortCutString,
                            QObject *parent = 0);
    virtual ~CommandLocator();

    void appendCommand(Core::Command *cmd);

    virtual QString displayName() const;
    virtual QString id() const;
    virtual Priority priority() const;
    virtual QList<FilterEntry> matchesFor(const QString &entry);
    virtual void accept(FilterEntry selection) const;
    virtual void refresh(QFutureInterface<void> &future);

public slots:
    void setEnabled(bool e);

private:
    CommandLocatorPrivate *d;
};

inline CommandLocator &operator<<(CommandLocator &locator, Core::Command *cmd)
{
    locator.appendCommand(cmd);
    return locator;
}

} // namespace Locator

#endif // COMMANDLOCATOR_H
