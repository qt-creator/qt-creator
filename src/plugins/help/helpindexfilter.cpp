/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#include "helpindexfilter.h"
#include "helpplugin.h"

#include <extensionsystem/pluginmanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/modemanager.h>

#include <QtHelp/QHelpEngine>
#include <QtHelp/QHelpIndexModel>

using namespace QuickOpen;
using namespace Help;
using namespace Help::Internal;

Q_DECLARE_METATYPE(IQuickOpenFilter*);

HelpIndexFilter::HelpIndexFilter(HelpPlugin *plugin, QHelpEngine *helpEngine):
    m_plugin(plugin),
    m_helpEngine(helpEngine),
    m_icon(QIcon())  // TODO: Put an icon next to the results
{
    setIncludedByDefault(false);
    setShortcutString("?");

    connect(m_helpEngine->indexModel(), SIGNAL(indexCreated()),
            this, SLOT(updateIndices()));
}

void HelpIndexFilter::updateIndices()
{
    const QString currentFilter = m_plugin->indexFilter();
    if (!currentFilter.isEmpty())
        m_plugin->setIndexFilter(QString());

    m_helpIndex = m_helpEngine->indexModel()->stringList();

    if (!currentFilter.isEmpty())
        m_plugin->setIndexFilter(currentFilter);
}

QString HelpIndexFilter::trName() const
{
    return tr("Help index");
}

QString HelpIndexFilter::name() const
{
    return QLatin1String("HelpIndexFilter");
}

IQuickOpenFilter::Priority HelpIndexFilter::priority() const
{
    return Medium;
}

QList<FilterEntry> HelpIndexFilter::matchesFor(const QString &entry)
{
    QList<FilterEntry> entries;
    foreach (const QString &string, m_helpIndex) {
        if (string.contains(entry, Qt::CaseInsensitive)) {
            FilterEntry entry(this, string, QVariant(), m_icon);
            entries.append(entry);
        }
    }
    return entries;
}

void HelpIndexFilter::accept(FilterEntry selection) const
{
    QMap<QString, QUrl> links = m_helpEngine->indexModel()->linksForKeyword(selection.displayName);
    if (links.size() == 1) {
        emit linkActivated(links.begin().value());
    } else if (!links.isEmpty()) {
        emit linksActivated(links, selection.displayName);
    }
}

void HelpIndexFilter::refresh(QFutureInterface<void> &future)
{
    Q_UNUSED(future);
    // Nothing to refresh
}
