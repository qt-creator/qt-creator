// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "extensionmanagerlegalnotice.h"

#include "extensionmanagersettings.h"
#include "extensionmanagertr.h"

#include <utils/infobar.h>

#include <coreplugin/icore.h>

#include <QGuiApplication>

using namespace Core;
using namespace Utils;

namespace ExtensionManager {

void setLegalNoticeVisible(bool visible)
{
    const char kEnableExternalRepo[] = "EnableExternalRepo";

    InfoBar *infoBar = ICore::popupInfoBar();

    if (!visible) {
        if (infoBar->containsInfo(kEnableExternalRepo))
            infoBar->removeInfo(kEnableExternalRepo);
        return;
    }
    if (Internal::settings().useExternalRepo() || !infoBar->canInfoBeAdded(kEnableExternalRepo))
        return;

    const QString text = Tr::tr("Qt Creator Extensions are available from configured online "
                                "sources, such as Qt Creator Extensions Store provided by "
                                "Qt Group, but also third-party provided sources. "
                                "Extensions for Qt Creator may be created and owned by "
                                "third-parties.\n"
                                "\n"
                                "You acknowledge that you download, install, or use Extensions "
                                "from the Qt Creator Extensions Store at your own discretion "
                                "and risk. All Qt Creator Extensions are provided \"as is\" "
                                "without warranties of any kind, and may be subject to "
                                "additional license terms imposed by their owners or licensors.\n"
                                "\n"
                                "You can manage the use of Extensions in Settings > Extensions.");

    InfoBarEntry info(kEnableExternalRepo, text, InfoBarEntry::GlobalSuppression::Disabled);
    info.setTitle(Tr::tr("Use %1 Extensions?").arg(QGuiApplication::applicationDisplayName()));
    info.setInfoType(InfoLabel::Information);
    info.addCustomButton(
        Tr::tr("Use"),
        [] { Internal::setUseExternalRepo(true); },
        {},
        InfoBarEntry::ButtonAction::SuppressPersistently);
    info.addCustomButton(
        Tr::tr("Do not use"),
        [] { Internal::setUseExternalRepo(false); },
        {},
        InfoBarEntry::ButtonAction::SuppressPersistently);
    infoBar->addInfo(info);
}

} // ExtensionManager
