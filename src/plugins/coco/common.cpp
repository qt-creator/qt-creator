// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "common.h"

#include "cocopluginconstants.h"

#include <coreplugin/messagemanager.h>

QString maybeQuote(const QString &str)
{
    if ((str.contains(' ') || str.contains('\t')) && !str.startsWith('"'))
        return '"' + str + '"';
    else
        return str;
}

void logSilently(const QString &msg)
{
    static const QString prefix = QString{"[%1] "}.arg(Coco::Constants::PROFILE_NAME);

    Core::MessageManager::writeSilently(prefix + msg);
}

void logFlashing(const QString &msg)
{
    static const QString prefix = QString{"[%1] "}.arg(Coco::Constants::PROFILE_NAME);

    Core::MessageManager::writeFlashing(prefix + msg);
}
