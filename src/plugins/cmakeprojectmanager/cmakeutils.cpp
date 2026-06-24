// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cmakeutils.h"

#include "cmakeprojectconstants.h"

#include <utils/algorithm.h>
#include <utils/stringutils.h>
#include <utils/theme/theme.h>

using namespace Utils;

namespace CMakeProjectManager::Internal {

QString addCMakePrefix(const QString &str)
{
    static const QString prefix
        = Utils::ansiColoredText(Constants::OUTPUT_PREFIX, creatorColor(Theme::Token_Text_Muted));
    return prefix + str;
}

QStringList addCMakePrefix(const QStringList &list)
{
    return Utils::transform(list, [](const QString &str) { return addCMakePrefix(str); });
}

} // CMakeProjectManager::Internal
