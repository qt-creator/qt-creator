// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "warningitem.h"

namespace ScxmlEditor {

namespace PluginInterface {

class TransitionItem;

/**
 * @brief The TransitionWarningItem class provides the warning item for the check Transition connections.
 */
class TransitionWarningItem : public WarningItem
{
    Q_OBJECT

public:
    TransitionWarningItem(TransitionItem *parent = nullptr);

    int type() const override
    {
        return TransitionWarningType;
    }

    void check() override;

private:
    TransitionItem *m_parentItem;
};

} // namespace PluginInterface
} // namespace ScxmlEditor
