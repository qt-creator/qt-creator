// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtGlobal>

#include "quick3drenderablenodeinstance.h"

QT_FORWARD_DECLARE_CLASS(QQuick3DNode)

namespace QmlDesigner {
namespace Internal {

class Quick3DNodeInstance : public Quick3DRenderableNodeInstance
{
public:
    using Pointer = QSharedPointer<Quick3DNodeInstance>;

    ~Quick3DNodeInstance() override;
    static Pointer create(QObject *objectToBeWrapped);
    void setHiddenInEditor(bool b) override;
    void initialize(const ObjectNodeInstance::Pointer &objectNodeInstance,
                    InstanceContainer::NodeFlags flags) override;

protected:
    explicit Quick3DNodeInstance(QObject *node);
    void invokeDummyViewCreate() const override;

private:
    QQuick3DNode *quick3DNode() const;
};

} // namespace Internal
} // namespace QmlDesigner
