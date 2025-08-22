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

void setLegalNoticeVisible(bool visible, const QString &text)
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

    const QString effectiveText =
        !text.isEmpty() ? text
                        : Tr::tr("The Extensions mode displays the Qt Creator Extensions "
                                 "available from configured online sources (such as Qt Creator "
                                 "Extension Store provided by Qt Group).\n\n"
                                 "If you choose to link or connect an external repository, you are "
                                 "acting at your own discretion and risk.\n\n"
                                 "By linking or connecting external repositories, you acknowledge "
                                 "these conditions and accept responsibility for managing "
                                 "associated risks appropriately.\n\n"
                                 "You can manage the use of Extensions in "
                                 "Preferences > Extensions.");

    InfoBarEntry
        info(kEnableExternalRepo, effectiveText,
             InfoBarEntry::GlobalSuppression::Disabled); // Custom buttons do SuppressPersistently
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
