// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include "nodedumper.h"

namespace QmlDesigner {
class ModelNode;
class Component;

class ItemNodeDumper : public NodeDumper
{
public:
    ItemNodeDumper(const QByteArrayList &lineage, const ModelNode &node);

    ~ItemNodeDumper() override = default;

    int priority() const override { return 100; }
    bool isExportable() const override;
    QJsonObject json(Component &component) const override;
};
}
