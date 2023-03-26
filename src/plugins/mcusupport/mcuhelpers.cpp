// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mcuhelpers.h"
#include "mcutargetdescription.h"

#include <QRegularExpression>

namespace McuSupport::Internal {

McuTarget::OS deduceOperatingSystem(const McuTargetDescription &desc)
{
    using OS = McuTarget::OS;
    using TargetType = McuTargetDescription::TargetType;
    if (desc.platform.type == TargetType::Desktop)
        return OS::Desktop;
    else if (!desc.freeRTOS.envVar.isEmpty())
        return OS::FreeRTOS;
    return OS::BareMetal;
}

QString removeRtosSuffix(const QString &environmentVariable)
{
    static const QRegularExpression freeRtosSuffix{R"(_FREERTOS_\w+)"};
    QString result = environmentVariable;
    return result.replace(freeRtosSuffix, QString{});
}

} // namespace McuSupport::Internal
