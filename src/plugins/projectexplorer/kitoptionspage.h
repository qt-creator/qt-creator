// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectexplorer_export.h"

#include <coreplugin/dialogs/ioptionspage.h>

namespace ProjectExplorer {

class Kit;

class PROJECTEXPLORER_EXPORT KitOptionsPage : public Core::IOptionsPage
{
public:
    KitOptionsPage();

    static void showKit(Kit *k);
};

} // namespace ProjectExplorer
