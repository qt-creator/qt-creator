/****************************************************************************
**
** Copyright (C) 2016 Nicolas Arnaud-Cormos
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

#include "macrolocatorfilter.h"
#include "macromanager.h"
#include "macro.h"

#include <coreplugin/icore.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>

#include <QPixmap>

using namespace Macros;
using namespace Macros::Internal;

MacroLocatorFilter::MacroLocatorFilter():
    m_icon(QPixmap(QLatin1String(":/macros/images/macro.png")))
{
    setId("Macros");
    setDisplayName(tr("Text Editing Macros"));
    setShortcutString(QLatin1String("rm"));
}

MacroLocatorFilter::~MacroLocatorFilter()
{
}

QList<Core::LocatorFilterEntry> MacroLocatorFilter::matchesFor(QFutureInterface<Core::LocatorFilterEntry> &future, const QString &entry)
{
    Q_UNUSED(future)
    QList<Core::LocatorFilterEntry> goodEntries;
    QList<Core::LocatorFilterEntry> betterEntries;

    const Qt::CaseSensitivity entryCaseSensitivity = caseSensitivity(entry);

    const QMap<QString, Macro*> &macros = MacroManager::macros();
    QMapIterator<QString, Macro*> it(macros);

    while (it.hasNext()) {
        it.next();
        const QString displayName = it.key();
        const QString description = it.value()->description();

        int index = displayName.indexOf(entry, 0, entryCaseSensitivity);
        Core::LocatorFilterEntry::HighlightInfo::DataType hDataType = Core::LocatorFilterEntry::HighlightInfo::DisplayName;
        if (index < 0) {
            index = description.indexOf(entry, 0, entryCaseSensitivity);
            hDataType = Core::LocatorFilterEntry::HighlightInfo::ExtraInfo;
        }

        if (index >= 0) {
            Core::LocatorFilterEntry filterEntry(this, displayName, QVariant(), m_icon);
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

void MacroLocatorFilter::accept(Core::LocatorFilterEntry selection,
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

void MacroLocatorFilter::refresh(QFutureInterface<void> &future)
{
    Q_UNUSED(future)
}
