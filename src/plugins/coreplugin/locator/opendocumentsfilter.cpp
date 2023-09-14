// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "opendocumentsfilter.h"

#include "../coreplugintr.h"
#include "../editormanager/documentmodel.h"

#include <extensionsystem/pluginmanager.h>

#include <utils/algorithm.h>
#include <utils/async.h>
#include <utils/link.h>

#include <QRegularExpression>

using namespace Tasking;
using namespace Utils;

namespace Core::Internal {

class Entry
{
public:
    Utils::FilePath fileName;
    QString displayName;
};

OpenDocumentsFilter::OpenDocumentsFilter()
{
    setId("Open documents");
    setDisplayName(Tr::tr("Open Documents"));
    setDescription(Tr::tr("Switches to an open document."));
    setDefaultShortcutString("o");
    setPriority(High);
    setDefaultIncludedByDefault(true);
}

static void matchEditors(QPromise<void> &promise, const LocatorStorage &storage,
                         const QList<Entry> &editorsData)
{
    const Link link = Link::fromString(storage.input(), true);
    const QRegularExpression regexp = ILocatorFilter::createRegExp(link.targetFilePath.toString());
    if (!regexp.isValid())
        return;

    LocatorFilterEntries goodEntries;
    LocatorFilterEntries betterEntries;

    for (const Entry &editorData : editorsData) {
        if (promise.isCanceled())
            return;
        if (editorData.fileName.isEmpty())
            continue;
        const QRegularExpressionMatch match = regexp.match(editorData.displayName);
        if (match.hasMatch()) {
            LocatorFilterEntry filterEntry;
            filterEntry.displayName = editorData.displayName;
            filterEntry.filePath = editorData.fileName;
            filterEntry.extraInfo = filterEntry.filePath.shortNativePath();
            filterEntry.highlightInfo = ILocatorFilter::highlightInfo(match);
            filterEntry.linkForEditor = Link(filterEntry.filePath, link.targetLine,
                                             link.targetColumn);
            if (match.capturedStart() == 0)
                betterEntries.append(filterEntry);
            else
                goodEntries.append(filterEntry);
        }
    }
    storage.reportOutput(betterEntries + goodEntries);
}

LocatorMatcherTasks OpenDocumentsFilter::matchers()
{
    TreeStorage<LocatorStorage> storage;

    const auto onSetup = [storage](Async<void> &async) {
        const QList<Entry> editorsData = Utils::transform(DocumentModel::entries(),
            [](const DocumentModel::Entry *e) { return Entry{e->filePath(), e->displayName()}; });
        async.setFutureSynchronizer(ExtensionSystem::PluginManager::futureSynchronizer());
        async.setConcurrentCallData(matchEditors, *storage, editorsData);
    };

    return {{AsyncTask<void>(onSetup), storage}};
}

} // namespace Core::Internal
