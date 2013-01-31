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

#include "qmljsfunctionfilter.h"
#include "qmljslocatordata.h"

#include <texteditor/itexteditor.h>
#include <texteditor/basetexteditor.h>

#include <QStringMatcher>

using namespace QmlJSTools::Internal;

Q_DECLARE_METATYPE(LocatorData::Entry)

FunctionFilter::FunctionFilter(LocatorData *data, QObject *parent)
    : Locator::ILocatorFilter(parent)
    , m_data(data)
{
    setId("Functions");
    setDisplayName(tr("QML Methods and Functions"));
    setShortcutString(QString(QLatin1Char('m')));
    setIncludedByDefault(false);
}

FunctionFilter::~FunctionFilter()
{ }

void FunctionFilter::refresh(QFutureInterface<void> &)
{
}

static bool compareLexigraphically(const Locator::FilterEntry &a,
                                   const Locator::FilterEntry &b)
{
    return a.displayName < b.displayName;
}

QList<Locator::FilterEntry> FunctionFilter::matchesFor(QFutureInterface<Locator::FilterEntry> &future, const QString &origEntry)
{
    QString entry = trimWildcards(origEntry);
    QList<Locator::FilterEntry> goodEntries;
    QList<Locator::FilterEntry> betterEntries;
    const QChar asterisk = QLatin1Char('*');
    QStringMatcher matcher(entry, Qt::CaseInsensitive);
    QRegExp regexp(asterisk + entry+ asterisk, Qt::CaseInsensitive, QRegExp::Wildcard);
    if (!regexp.isValid())
        return goodEntries;
    bool hasWildcard = (entry.contains(asterisk) || entry.contains(QLatin1Char('?')));

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
                Locator::FilterEntry filterEntry(this, info.displayName, id/*, info.icon*/);
                filterEntry.extraInfo = info.extraInfo;

                if (info.symbolName.startsWith(entry))
                    betterEntries.append(filterEntry);
                else
                    goodEntries.append(filterEntry);
            }
        }
    }

    if (goodEntries.size() < 1000)
        qSort(goodEntries.begin(), goodEntries.end(), compareLexigraphically);
    if (betterEntries.size() < 1000)
        qSort(betterEntries.begin(), betterEntries.end(), compareLexigraphically);

    betterEntries += goodEntries;
    return betterEntries;
}

void FunctionFilter::accept(Locator::FilterEntry selection) const
{
    const LocatorData::Entry entry = qvariant_cast<LocatorData::Entry>(selection.internalData);
    TextEditor::BaseTextEditorWidget::openEditorAt(entry.fileName, entry.line, entry.column,
                                             Core::Id(), Core::EditorManager::ModeSwitch);
}
