/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

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
