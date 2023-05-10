// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "externaltoolsfilter.h"

#include "../coreconstants.h"
#include "../coreplugin.h"
#include "../coreplugintr.h"
#include "../externaltool.h"
#include "../externaltoolmanager.h"
#include "../icore.h"
#include "../messagemanager.h"

#include <utils/qtcassert.h>

namespace Core::Internal {

ExternalToolsFilter::ExternalToolsFilter()
{
    setId("Run external tool");
    setDisplayName(Tr::tr("Run External Tool"));
    setDescription(Tr::tr("Runs an external tool that you have set up in the preferences "
                          "(Environment > External Tools)."));
    setDefaultShortcutString("x");
    setPriority(Medium);
}

LocatorMatcherTasks ExternalToolsFilter::matchers()
{
    using namespace Tasking;

    TreeStorage<LocatorStorage> storage;

    const auto onSetup = [storage] {
        const QString input = storage->input();

        LocatorFilterEntries bestEntries;
        LocatorFilterEntries betterEntries;
        LocatorFilterEntries goodEntries;
        const Qt::CaseSensitivity entryCaseSensitivity = caseSensitivity(input);
        const QMap<QString, ExternalTool *> externalToolsById = ExternalToolManager::toolsById();
        for (ExternalTool *tool : externalToolsById) {
            int index = tool->displayName().indexOf(input, 0, entryCaseSensitivity);
            LocatorFilterEntry::HighlightInfo::DataType hDataType = LocatorFilterEntry::HighlightInfo::DisplayName;
            if (index < 0) {
                index = tool->description().indexOf(input, 0, entryCaseSensitivity);
                hDataType = LocatorFilterEntry::HighlightInfo::ExtraInfo;
            }

            if (index >= 0) {
                LocatorFilterEntry filterEntry;
                filterEntry.displayName = tool->displayName();
                filterEntry.acceptor = [tool] {
                    auto runner = new ExternalToolRunner(tool);
                    if (runner->hasError())
                        MessageManager::writeFlashing(runner->errorString());
                    return AcceptResult();
                };
                filterEntry.extraInfo = tool->description();
                filterEntry.highlightInfo = LocatorFilterEntry::HighlightInfo(index, input.length(), hDataType);

                if (filterEntry.displayName.startsWith(input, entryCaseSensitivity))
                    bestEntries.append(filterEntry);
                else if (filterEntry.displayName.contains(input, entryCaseSensitivity))
                    betterEntries.append(filterEntry);
                else
                    goodEntries.append(filterEntry);
            }
        }
        LocatorFilterEntry configEntry;
        configEntry.displayName = "Configure External Tool...";
        configEntry.extraInfo = "Opens External Tool settings";
        configEntry.acceptor = [] {
            QMetaObject::invokeMethod(CorePlugin::instance(), [] {
                ICore::showOptionsDialog(Constants::SETTINGS_ID_TOOLS);
            }, Qt::QueuedConnection);
            return AcceptResult();
        };

        storage->reportOutput(bestEntries + betterEntries + goodEntries
                              + LocatorFilterEntries{configEntry});
    };
    return {{Sync(onSetup), storage}};
}

} // Core::Internal
