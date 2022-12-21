// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "graphicsitemprovider.h"

namespace ScxmlEditor {

namespace PluginInterface {

class SCGraphicsItemProvider : public GraphicsItemProvider
{
public:
    SCGraphicsItemProvider(QObject *parent = nullptr);

    WarningItem *createWarningItem(const QString &key, BaseItem *parentItem) const override;
};

} // namespace PluginInterface
} // namespace ScxmlEditor
