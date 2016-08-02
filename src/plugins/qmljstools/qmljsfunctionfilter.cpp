/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "qmljsfunctionfilter.h"
#include "qmljslocatordata.h"

#include <coreplugin/editormanager/editormanager.h>
#include <utils/algorithm.h>

#include <QRegExp>
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

QList<Core::LocatorFilterEntry> FunctionFilter::matchesFor(
        QFutureInterface<Core::LocatorFilterEntry> &future,
        const QString &entry)
{
    QList<Core::LocatorFilterEntry> goodEntries;
    QList<Core::LocatorFilterEntry> betterEntries;
    const Qt::CaseSensitivity cs = caseSensitivity(entry);
    QStringMatcher matcher(entry, cs);
    QRegExp regexp(entry, cs, QRegExp::Wildcard);
    if (!regexp.isValid())
        return goodEntries;
    bool hasWildcard = containsWildcard(entry);

    QHashIterator<QString, QList<LocatorData::Entry> > it(m_data->entries());
    while (it.hasNext()) {
        if (future.isCanceled())
            break;

        it.next();

        const QList<LocatorData::Entry> items = it.value();
        foreach (const LocatorData::Entry &info, items) {
            if (info.type != LocatorData::Function)
                continue;

            const int index = hasWildcard ? regexp.indexIn(info.symbolName)
                                          : matcher.indexIn(info.symbolName);
            if (index >= 0) {
                QVariant id = qVariantFromValue(info);
                Core::LocatorFilterEntry filterEntry(this, info.displayName, id/*, info.icon*/);
                filterEntry.extraInfo = info.extraInfo;
                const int length = hasWildcard ? regexp.matchedLength() : entry.length();
                filterEntry.highlightInfo = {index, length};

                if (index == 0)
                    betterEntries.append(filterEntry);
                else
                    goodEntries.append(filterEntry);
            }
        }
    }

    if (goodEntries.size() < 1000)
        Utils::sort(goodEntries, Core::LocatorFilterEntry::compareLexigraphically);
    if (betterEntries.size() < 1000)
        Utils::sort(betterEntries, Core::LocatorFilterEntry::compareLexigraphically);

    betterEntries += goodEntries;
    return betterEntries;
}

void FunctionFilter::accept(Core::LocatorFilterEntry selection) const
{
    const LocatorData::Entry entry = qvariant_cast<LocatorData::Entry>(selection.internalData);
    Core::EditorManager::openEditorAt(entry.fileName, entry.line, entry.column);
}
