/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include "helpindexfilter.h"

#include "centralwidget.h"
#include "topicchooser.h"

#include <extensionsystem/pluginmanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/helpmanager.h>

#include <QIcon>

using namespace Locator;
using namespace Help;
using namespace Help::Internal;

Q_DECLARE_METATYPE(ILocatorFilter*)

HelpIndexFilter::HelpIndexFilter()
{
    setId("HelpIndexFilter");
    setDisplayName(tr("Help Index"));
    setIncludedByDefault(false);
    setShortcutString(QString(QLatin1Char('?')));

    m_icon = QIcon(QLatin1String(":/help/images/bookmark.png"));
}

HelpIndexFilter::~HelpIndexFilter()
{
}

QList<FilterEntry> HelpIndexFilter::matchesFor(QFutureInterface<Locator::FilterEntry> &future, const QString &entry)
{
    QStringList keywords;
    if (entry.length() < 2)
        keywords = Core::HelpManager::instance()->findKeywords(entry, 200);
    else
        keywords = Core::HelpManager::instance()->findKeywords(entry);

    QList<FilterEntry> entries;
    foreach (const QString &keyword, keywords) {
        if (future.isCanceled())
            break;
        entries.append(FilterEntry(this, keyword, QVariant(), m_icon));
    }

    return entries;
}

void HelpIndexFilter::accept(FilterEntry selection) const
{
    const QString &key = selection.displayName;
    const QMap<QString, QUrl> &links = Core::HelpManager::instance()->linksForKeyword(key);

    if (links.size() == 1) {
        emit linkActivated(links.begin().value());
    } else if (!links.isEmpty()) {
        TopicChooser tc(CentralWidget::instance(), key, links);
        if (tc.exec() == QDialog::Accepted)
            emit linkActivated(tc.link());
    }
}

void HelpIndexFilter::refresh(QFutureInterface<void> &future)
{
    Q_UNUSED(future)
    // Nothing to refresh
}
