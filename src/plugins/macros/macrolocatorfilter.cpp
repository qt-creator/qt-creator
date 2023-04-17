// Copyright (C) 2016 Nicolas Arnaud-Cormos
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "macrolocatorfilter.h"

#include "macro.h"
#include "macromanager.h"
#include "macrostr.h"

#include <coreplugin/editormanager/editormanager.h>

#include <QPixmap>

using namespace Core;
using namespace Macros;
using namespace Macros::Internal;
using namespace Utils;

MacroLocatorFilter::MacroLocatorFilter()
    : m_icon(QPixmap(":/macros/images/macro.png"))
{
    setId("Macros");
    setDisplayName(Tr::tr("Text Editing Macros"));
    setDescription(Tr::tr("Runs a text editing macro that was recorded with Tools > Text Editing "
                          "Macros > Record Macro."));
    setDefaultShortcutString("rm");
}

LocatorMatcherTasks MacroLocatorFilter::matchers()
{
    using namespace Tasking;

    TreeStorage<LocatorStorage> storage;

    const auto onSetup = [storage, icon = m_icon] {
        const QString input = storage->input();
        const Qt::CaseSensitivity entryCaseSensitivity = caseSensitivity(input);
        const QMap<QString, Macro *> &macros = MacroManager::macros();
        LocatorFilterEntries goodEntries;
        LocatorFilterEntries betterEntries;
        for (auto it = macros.cbegin(); it != macros.cend(); ++it) {
            const QString displayName = it.key();
            const QString description = it.value()->description();
            int index = displayName.indexOf(input, 0, entryCaseSensitivity);
            LocatorFilterEntry::HighlightInfo::DataType hDataType
                = LocatorFilterEntry::HighlightInfo::DisplayName;
            if (index < 0) {
                index = description.indexOf(input, 0, entryCaseSensitivity);
                hDataType = LocatorFilterEntry::HighlightInfo::ExtraInfo;
            }

            if (index >= 0) {
                LocatorFilterEntry filterEntry;
                filterEntry.displayName = displayName;
                filterEntry.acceptor = [displayName] {
                    IEditor *editor = EditorManager::currentEditor();
                    if (editor)
                        editor->widget()->setFocus(Qt::OtherFocusReason);
                    MacroManager::instance()->executeMacro(displayName);
                    return AcceptResult();
                };
                filterEntry.displayIcon = icon;
                filterEntry.extraInfo = description;
                filterEntry.highlightInfo = LocatorFilterEntry::HighlightInfo(index, input.length(),
                                                                              hDataType);
                if (index == 0)
                    betterEntries.append(filterEntry);
                else
                    goodEntries.append(filterEntry);
            }
        }
        storage->reportOutput(betterEntries + goodEntries);
        return true;
    };

    return {{Sync(onSetup), storage}};
}

QList<LocatorFilterEntry> MacroLocatorFilter::matchesFor(QFutureInterface<LocatorFilterEntry> &future, const QString &entry)
{
    Q_UNUSED(future)
    QList<LocatorFilterEntry> goodEntries;
    QList<LocatorFilterEntry> betterEntries;

    const Qt::CaseSensitivity entryCaseSensitivity = caseSensitivity(entry);

    const QMap<QString, Macro*> &macros = MacroManager::macros();

    for (auto it = macros.cbegin(), end = macros.cend(); it != end; ++it) {
        const QString displayName = it.key();
        const QString description = it.value()->description();

        int index = displayName.indexOf(entry, 0, entryCaseSensitivity);
        LocatorFilterEntry::HighlightInfo::DataType hDataType
            = LocatorFilterEntry::HighlightInfo::DisplayName;
        if (index < 0) {
            index = description.indexOf(entry, 0, entryCaseSensitivity);
            hDataType = LocatorFilterEntry::HighlightInfo::ExtraInfo;
        }

        if (index >= 0) {
            LocatorFilterEntry filterEntry;
            filterEntry.displayName = displayName;
            filterEntry.acceptor = [displayName] {
                IEditor *editor = EditorManager::currentEditor();
                if (editor)
                    editor->widget()->setFocus(Qt::OtherFocusReason);

                MacroManager::instance()->executeMacro(displayName);
                return AcceptResult();
            };
            filterEntry.displayIcon = m_icon;
            filterEntry.extraInfo = description;
            filterEntry.highlightInfo = LocatorFilterEntry::HighlightInfo(index, entry.length(), hDataType);

            if (index == 0)
                betterEntries.append(filterEntry);
            else
                goodEntries.append(filterEntry);
        }
    }
    betterEntries.append(goodEntries);
    return betterEntries;
}
