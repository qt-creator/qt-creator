// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectexplorer_export.h"

#include <coreplugin/dialogs/ioptionspage.h>

#include <QPointer>

namespace ProjectExplorer {

namespace Internal { class KitOptionsPageWidget; }

class Kit;

class PROJECTEXPLORER_EXPORT KitOptionsPage : public Core::IOptionsPage
{
public:
    KitOptionsPage();

    void showKit(Kit *k);
    static KitOptionsPage *instance();
};

} // namespace ProjectExplorer
