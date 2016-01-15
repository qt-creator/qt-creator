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

#include <coreplugin/externaltool.h>
#include <coreplugin/externaltoolmanager.h>
#include <coreplugin/messagemanager.h>
#include <utils/qtcassert.h>

#include "externaltoolsfilter.h"

using namespace Core;
using namespace Core::Internal;

ExternalToolsFilter::ExternalToolsFilter()
{
    setId("Run external tool");
    setDisplayName(tr("Run External Tool"));
    setShortcutString(QLatin1String("x"));
    setPriority(Medium);
}

QList<LocatorFilterEntry> ExternalToolsFilter::matchesFor(QFutureInterface<LocatorFilterEntry> &,
                                                          const QString &)
{
    return m_results;
}

void ExternalToolsFilter::accept(LocatorFilterEntry selection) const
{
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
    m_results.clear();

    Qt::CaseSensitivity useCaseSensitivity = caseSensitivity(entry);
    const QMap<QString, ExternalTool *> externalToolsById = ExternalToolManager::toolsById();
    auto end = externalToolsById.cend();
    for (auto it = externalToolsById.cbegin(); it != end; ++it) {
        ExternalTool *tool = *it;
        if (tool->description().contains(entry, useCaseSensitivity) ||
                tool->displayName().contains(entry, useCaseSensitivity)) {

            LocatorFilterEntry filterEntry(this, tool->displayName(), QVariant::fromValue(tool));
            filterEntry.extraInfo = tool->description();
            m_results.append(filterEntry);
        }
    }
}
