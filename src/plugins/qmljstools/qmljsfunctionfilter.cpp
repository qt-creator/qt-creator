// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmljsfunctionfilter.h"
#include "qmljslocatordata.h"
#include "qmljstoolstr.h"

#include <coreplugin/editormanager/editormanager.h>
#include <utils/algorithm.h>

#include <QRegularExpression>

#include <numeric>

using namespace QmlJSTools::Internal;

Q_DECLARE_METATYPE(LocatorData::Entry)

FunctionFilter::FunctionFilter(LocatorData *data, QObject *parent)
    : Core::ILocatorFilter(parent)
    , m_data(data)
{
    setId("Functions");
    setDisplayName(Tr::tr("QML Functions"));
    setDefaultShortcutString("m");
    setDefaultIncludedByDefault(false);
}

FunctionFilter::~FunctionFilter() = default;

QList<Core::LocatorFilterEntry> FunctionFilter::matchesFor(
        QFutureInterface<Core::LocatorFilterEntry> &future,
        const QString &entry)
{
    QList<Core::LocatorFilterEntry> entries[int(MatchLevel::Count)];
    const Qt::CaseSensitivity caseSensitivityForPrefix = caseSensitivity(entry);

    const QRegularExpression regexp = createRegExp(entry);
    if (!regexp.isValid())
        return {};

    const QHash<Utils::FilePath, QList<LocatorData::Entry>> locatorEntries = m_data->entries();
    for (const QList<LocatorData::Entry> &items : locatorEntries) {
        if (future.isCanceled())
            break;

        for (const LocatorData::Entry &info : items) {
            if (info.type != LocatorData::Function)
                continue;

            const QRegularExpressionMatch match = regexp.match(info.symbolName);
            if (match.hasMatch()) {
                QVariant id = QVariant::fromValue(info);
                Core::LocatorFilterEntry filterEntry(this, info.displayName, id/*, info.icon*/);
                filterEntry.extraInfo = info.extraInfo;
                filterEntry.highlightInfo = highlightInfo(match);

                if (filterEntry.displayName.startsWith(entry, caseSensitivityForPrefix))
                    entries[int(MatchLevel::Best)].append(filterEntry);
                else if (filterEntry.displayName.contains(entry, caseSensitivityForPrefix))
                    entries[int(MatchLevel::Better)].append(filterEntry);
                else
                    entries[int(MatchLevel::Good)].append(filterEntry);
            }
        }
    }

    for (auto &entry : entries) {
        if (entry.size() < 1000)
            Utils::sort(entry, Core::LocatorFilterEntry::compareLexigraphically);
    }

    return std::accumulate(std::begin(entries), std::end(entries), QList<Core::LocatorFilterEntry>());
}

void FunctionFilter::accept(const Core::LocatorFilterEntry &selection,
                            QString *newText, int *selectionStart, int *selectionLength) const
{
    Q_UNUSED(newText)
    Q_UNUSED(selectionStart)
    Q_UNUSED(selectionLength)
    const LocatorData::Entry entry = qvariant_cast<LocatorData::Entry>(selection.internalData);
    Core::EditorManager::openEditorAt({entry.fileName, entry.line, entry.column});
}
