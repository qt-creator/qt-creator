// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include "abstractformeditortool.h"

namespace QmlDesigner {

class QMLDESIGNERCOMPONENTS_EXPORT AbstractCustomTool : public QmlDesigner::AbstractFormEditorTool
{
public:
    AbstractCustomTool();

    void selectedItemsChanged(const QList<FormEditorItem *> &itemList) override;

    virtual QString name() const = 0;

    virtual int wantHandleItem(const ModelNode &modelNode) const = 0;

    void focusLost() override;
};

} // namespace QmlDesigner
