// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "extensionsystem_global.h"

#include <QDialog>

namespace ExtensionSystem {

class EXTENSIONSYSTEM_EXPORT PluginErrorOverview : public QDialog
{
public:
    explicit PluginErrorOverview(QWidget *parent = nullptr);
};

} // ExtensionSystem
