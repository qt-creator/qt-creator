/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "qmljsfunctionfilter.h"
#include "qmljslocatordata.h"

#include <coreplugin/editormanager/editormanager.h>
#include <utils/algorithm.h>

#include <QStringMatcher>

using namespace QmlJSTools::Internal;

Q_DECLARE_METATYPE(LocatorData::Entry)

FunctionFilter::FunctionFilter(LocatorData *data, QObject *parent)
    : Core::ILocatorFilter(parent)
    , m_data(data)
{
    setId("Functions");
    setDisplayName(tr("QML Functions"));
    setShortcutString(QString(QLatin1Char('m')));
    setIncludedByDefault(false);
}

FunctionFilter::~FunctionFilter()
{ }

void FunctionFilter::refresh(QFutureInterface<void> &)
{
}

static bool compareLexigraphically(const Core::LocatorFilterEntry &a,
                                   const Core::LocatorFilterEntry &b)
{
    return a.displayName < b.displayName;
}

QList<Core::LocatorFilterEntry> FunctionFilter::matchesFor(QFutureInterface<Core::LocatorFilterEntry> &future, const QString &origEntry)
{
    QString entry = trimWildcards(origEntry);
    QList<Core::LocatorFilterEntry> goodEntries;
    QList<Core::LocatorFilterEntry> betterEntries;
    const QChar asterisk = QLatin1Char('*');
    QStringMatcher matcher(entry, Qt::CaseInsensitive);
    QRegExp regexp(asterisk + entry+ asterisk, Qt::CaseInsensitive, QRegExp::Wildcard);
    if (!regexp.isValid())
        return goodEntries;
    bool hasWildcard = (entry.contains(asterisk) || entry.contains(QLatin1Char('?')));
    const Qt::CaseSensitivity caseSensitivityForPrefix = caseSensitivity(entry);

    QHashIterator<QString, QList<LocatorData::Entry> > it(m_data->entries());
    while (it.hasNext()) {
        if (future.isCanceled())
            break;

        it.next();

        const QList<LocatorData::Entry> items = it.value();
        foreach (const LocatorData::Entry &info, items) {
            if (info.type != LocatorData::Function)
                continue;
            if ((hasWildcard && regexp.exactMatch(info.symbolName))
                    || (!hasWildcard && matcher.indexIn(info.symbolName) != -1)) {

                QVariant id = qVariantFromValue(info);
                Core::LocatorFilterEntry filterEntry(this, info.displayName, id/*, info.icon*/);
                filterEntry.extraInfo = info.extraInfo;

                if (info.symbolName.startsWith(entry, caseSensitivityForPrefix))
                    betterEntries.append(filterEntry);
                else
                    goodEntries.append(filterEntry);
            }
        }
    }

    if (goodEntries.size() < 1000)
        Utils::sort(goodEntries, compareLexigraphically);
    if (betterEntries.size() < 1000)
        Utils::sort(betterEntries, compareLexigraphically);

    betterEntries += goodEntries;
    return betterEntries;
}

void FunctionFilter::accept(Core::LocatorFilterEntry selection) const
{
    const LocatorData::Entry entry = qvariant_cast<LocatorData::Entry>(selection.internalData);
    Core::EditorManager::openEditorAt(entry.fileName, entry.line, entry.column);
}
