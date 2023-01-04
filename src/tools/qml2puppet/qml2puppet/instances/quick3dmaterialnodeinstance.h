// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtGlobal>

#include "quick3drenderablenodeinstance.h"

QT_FORWARD_DECLARE_CLASS(QQuick3DMaterial)

namespace QmlDesigner {
namespace Internal {

class Quick3DMaterialNodeInstance : public Quick3DRenderableNodeInstance
{
public:
    using Pointer = QSharedPointer<Quick3DMaterialNodeInstance>;

    ~Quick3DMaterialNodeInstance() override;
    static Pointer create(QObject *objectToBeWrapped);

protected:
    explicit Quick3DMaterialNodeInstance(QObject *node);
    void invokeDummyViewCreate() const override;
};

} // namespace Internal
} // namespace QmlDesigner
