// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "dynamicpropertiesproxymodel.h"

namespace QmlDesigner {

class TextureEditorDynamicPropertiesProxyModel : public DynamicPropertiesProxyModel
{
    Q_OBJECT

public:
    explicit TextureEditorDynamicPropertiesProxyModel(QObject *parent = nullptr);

    static void registerDeclarativeType();
};

} // namespace QmlDesigner
