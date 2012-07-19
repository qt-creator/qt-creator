/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nicolas Arnaud-Cormos.
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#ifndef MACROSPLUGIN_MACROLOCATORFILTER_H
#define MACROSPLUGIN_MACROLOCATORFILTER_H

#include <locator/ilocatorfilter.h>

#include <QIcon>

namespace Macros {

class MacroManager;

namespace Internal {

class MacroLocatorFilter : public Locator::ILocatorFilter
{
    Q_OBJECT

public:
    MacroLocatorFilter();
    ~MacroLocatorFilter();

    QString displayName() const { return tr("Macros"); }
    QString id() const { return QLatin1String("Macros"); }
    Priority priority() const { return Medium; }

    QList<Locator::FilterEntry> matchesFor(QFutureInterface<Locator::FilterEntry> &future, const QString &entry);
    void accept(Locator::FilterEntry selection) const;
    void refresh(QFutureInterface<void> &future);

private:
    const QIcon m_icon;
};

} // namespace Internal
} // namespace Macros

#endif // MACROSPLUGIN_MACROLOCATORFILTER_H
