/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include "helpindexfilter.h"

#include <extensionsystem/pluginmanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/helpmanager.h>

#include <QtGui/QIcon>

using namespace Locator;
using namespace Help;
using namespace Help::Internal;

Q_DECLARE_METATYPE(ILocatorFilter*);

HelpIndexFilter::HelpIndexFilter()
{
    setIncludedByDefault(false);
    setShortcutString(QString(QLatin1Char('?')));
    m_icon = QIcon(QLatin1String(":/help/images/bookmark.png"));
}

HelpIndexFilter::~HelpIndexFilter()
{
}

QString HelpIndexFilter::displayName() const
{
    return tr("Help index");
}

QString HelpIndexFilter::id() const
{
    return QLatin1String("HelpIndexFilter");
}

ILocatorFilter::Priority HelpIndexFilter::priority() const
{
    return Medium;
}

QList<FilterEntry> HelpIndexFilter::matchesFor(const QString &entry)
{
    QStringList keywords;
    if (entry.length() < 2)
        keywords = Core::HelpManager::instance()->findKeywords(entry, 300);
    else
        keywords = Core::HelpManager::instance()->findKeywords(entry);

    QList<FilterEntry> entries;
    foreach (const QString &keyword, keywords)
        entries.append(FilterEntry(this, keyword, QVariant(), m_icon));

    return entries;
}

void HelpIndexFilter::accept(FilterEntry selection) const
{
    const QString &key = selection.displayName;
    const QMap<QString, QUrl> &links = Core::HelpManager::instance()->linksForKeyword(key);
    if (links.size() == 1) {
        emit linkActivated(links.begin().value());
    } else if (!links.isEmpty()) {
        emit linksActivated(links, key);
    }
}

void HelpIndexFilter::refresh(QFutureInterface<void> &future)
{
    Q_UNUSED(future)
    // Nothing to refresh
}
