// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "warningitem.h"

namespace ScxmlEditor {

namespace PluginInterface {

class IdWarningItem : public WarningItem
{
    Q_OBJECT

public:
    IdWarningItem(QGraphicsItem *parent = nullptr);

    int type() const override
    {
        return IdWarningType;
    }

    QString id() const
    {
        return m_id;
    }

    void check() override;
    void setId(const QString &text);

private:
    void checkDuplicates(const QString &id);

    QString m_id;
};

} // namespace PluginInterface
} // namespace ScxmlEditor
