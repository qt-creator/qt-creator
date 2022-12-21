// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utilsprovider.h"

namespace ScxmlEditor {

namespace PluginInterface {

class ScxmlTag;

class SCUtilsProvider : public UtilsProvider
{
public:
    SCUtilsProvider(QObject *parent = nullptr);

    void checkInitialState(const QList<QGraphicsItem*> &items, ScxmlTag *parentTag) override;
};

} // namespace PluginInterface
} // namespace ScxmlEditor
