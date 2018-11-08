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

#include "externaltoolsfilter.h"

#include <coreplugin/externaltool.h>
#include <coreplugin/externaltoolmanager.h>
#include <coreplugin/messagemanager.h>
#include <utils/qtcassert.h>

using namespace Core;
using namespace Core::Internal;

ExternalToolsFilter::ExternalToolsFilter()
{
    setId("Run external tool");
    setDisplayName(tr("Run External Tool"));
    setShortcutString("x");
    setPriority(Medium);
}

QList<LocatorFilterEntry> ExternalToolsFilter::matchesFor(QFutureInterface<LocatorFilterEntry> &,
                                                          const QString &)
{
    return m_results;
}

void ExternalToolsFilter::accept(LocatorFilterEntry selection,
                                 QString *newText, int *selectionStart, int *selectionLength) const
{
    Q_UNUSED(newText)
    Q_UNUSED(selectionStart)
    Q_UNUSED(selectionLength)
    auto tool = selection.internalData.value<ExternalTool *>();
    QTC_ASSERT(tool, return);

    auto runner = new ExternalToolRunner(tool);
    if (runner->hasError())
        MessageManager::write(runner->errorString());
}

void ExternalToolsFilter::refresh(QFutureInterface<void> &)
{
}

void ExternalToolsFilter::prepareSearch(const QString &entry)
{
    QList<LocatorFilterEntry> bestEntries;
    QList<LocatorFilterEntry> betterEntries;
    QList<LocatorFilterEntry> goodEntries;
    const Qt::CaseSensitivity entryCaseSensitivity = caseSensitivity(entry);
    const QMap<QString, ExternalTool *> externalToolsById = ExternalToolManager::toolsById();
    for (ExternalTool *tool : externalToolsById) {
        int index = tool->displayName().indexOf(entry, 0, entryCaseSensitivity);
        LocatorFilterEntry::HighlightInfo::DataType hDataType = LocatorFilterEntry::HighlightInfo::DisplayName;
        if (index < 0) {
            index = tool->description().indexOf(entry, 0, entryCaseSensitivity);
            hDataType = LocatorFilterEntry::HighlightInfo::ExtraInfo;
        }

        if (index >= 0) {
            LocatorFilterEntry filterEntry(this, tool->displayName(), QVariant::fromValue(tool));
            filterEntry.extraInfo = tool->description();
            filterEntry.highlightInfo = LocatorFilterEntry::HighlightInfo(index, entry.length(), hDataType);

            if (filterEntry.displayName.startsWith(entry, entryCaseSensitivity))
                bestEntries.append(filterEntry);
            else if (filterEntry.displayName.contains(entry, entryCaseSensitivity))
                betterEntries.append(filterEntry);
            else
                goodEntries.append(filterEntry);
        }
    }
    m_results = bestEntries + betterEntries + goodEntries;
}
