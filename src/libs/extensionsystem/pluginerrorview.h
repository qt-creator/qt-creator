// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "extensionsystem_global.h"

#include <QWidget>

namespace ExtensionSystem {

class PluginSpec;
namespace Internal { class PluginErrorViewPrivate; }

class EXTENSIONSYSTEM_EXPORT PluginErrorView : public QWidget
{
    Q_OBJECT

public:
    PluginErrorView(QWidget *parent = nullptr);
    ~PluginErrorView() override;

    void update(PluginSpec *spec);

private:
    Internal::PluginErrorViewPrivate *d = nullptr;
};

} // namespace ExtensionSystem
