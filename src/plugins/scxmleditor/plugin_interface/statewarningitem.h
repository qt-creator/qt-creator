// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "stateitem.h"
#include "warningitem.h"
#include <QPointer>

namespace ScxmlEditor {

namespace PluginInterface {

class IdWarningItem;

/**
 * @brief The StateWarningItem class
 */
class StateWarningItem : public WarningItem
{
    Q_OBJECT

public:
    StateWarningItem(StateItem *parent = nullptr);

    void setIdWarning(IdWarningItem *idwarning);

    int type() const override
    {
        return StateWarningType;
    }

    void check() override;

private:
    QPointer<IdWarningItem> m_idWarningItem;
    StateItem *m_parentItem = nullptr;
};

} // namespace PluginInterface
} // namespace ScxmlEditor
