// Copyright (C) 2016 Nicolas Arnaud-Cormos
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "macrolocatorfilter.h"

#include "macro.h"
#include "macromanager.h"
#include "macrostr.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/icore.h>

#include <QPixmap>

using namespace Macros;
using namespace Macros::Internal;

MacroLocatorFilter::MacroLocatorFilter()
    : m_icon(QPixmap(":/macros/images/macro.png"))
{
    setId("Macros");
    setDisplayName(Tr::tr("Text Editing Macros"));
    setDescription(Tr::tr("Runs a text editing macro that was recorded with Tools > Text Editing "
                          "Macros > Record Macro."));
    setDefaultShortcutString("rm");
}

MacroLocatorFilter::~MacroLocatorFilter() = default;

QList<Core::LocatorFilterEntry> MacroLocatorFilter::matchesFor(QFutureInterface<Core::LocatorFilterEntry> &future, const QString &entry)
{
    Q_UNUSED(future)
    QList<Core::LocatorFilterEntry> goodEntries;
    QList<Core::LocatorFilterEntry> betterEntries;

    const Qt::CaseSensitivity entryCaseSensitivity = caseSensitivity(entry);

    const QMap<QString, Macro*> &macros = MacroManager::macros();

    for (auto it = macros.cbegin(), end = macros.cend(); it != end; ++it) {
        const QString displayName = it.key();
        const QString description = it.value()->description();

        int index = displayName.indexOf(entry, 0, entryCaseSensitivity);
        Core::LocatorFilterEntry::HighlightInfo::DataType hDataType = Core::LocatorFilterEntry::HighlightInfo::DisplayName;
        if (index < 0) {
            index = description.indexOf(entry, 0, entryCaseSensitivity);
            hDataType = Core::LocatorFilterEntry::HighlightInfo::ExtraInfo;
        }

        if (index >= 0) {
            Core::LocatorFilterEntry filterEntry(this, displayName);
            filterEntry.displayIcon = m_icon;
            filterEntry.extraInfo = description;
            filterEntry.highlightInfo = Core::LocatorFilterEntry::HighlightInfo(index, entry.length(), hDataType);

            if (index == 0)
                betterEntries.append(filterEntry);
            else
                goodEntries.append(filterEntry);
        }
    }
    betterEntries.append(goodEntries);
    return betterEntries;
}

void MacroLocatorFilter::accept(const Core::LocatorFilterEntry &selection,
                                QString *newText, int *selectionStart, int *selectionLength) const
{
    Q_UNUSED(newText)
    Q_UNUSED(selectionStart)
    Q_UNUSED(selectionLength)
    // Give the focus back to the editor
    Core::IEditor *editor = Core::EditorManager::currentEditor();
    if (editor)
        editor->widget()->setFocus(Qt::OtherFocusReason);

    MacroManager::instance()->executeMacro(selection.displayName);
}
