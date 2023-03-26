// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include "itemnodedumper.h"

namespace QmlDesigner {
class Component;

class TextNodeDumper : public ItemNodeDumper
{
public:
    TextNodeDumper(const QByteArrayList &lineage, const ModelNode &node);
    ~TextNodeDumper() override = default;

    bool isExportable() const override;
    int priority() const override { return 200; }
    QJsonObject json(Component &component) const override;
};

}
