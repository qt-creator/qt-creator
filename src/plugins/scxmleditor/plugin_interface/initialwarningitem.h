// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "warningitem.h"

namespace ScxmlEditor {

namespace PluginInterface {

class InitialStateItem;

class InitialWarningItem : public WarningItem
{
    Q_OBJECT
public:
    InitialWarningItem(InitialStateItem *parent = nullptr);

    int type() const override
    {
        return InitialWarningType;
    }

    void check() override;

    void updatePos();

private:
    InitialStateItem *m_parentItem;
};

} // namespace PluginInterface
} // namespace ScxmlEditor
