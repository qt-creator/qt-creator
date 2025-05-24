// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmlmodelnodefacade.h"

#include <signalhandlerproperty.h>

namespace QmlDesigner {

class StatesEditorView;

class QMLDESIGNER_EXPORT QmlConnections : public QmlModelNodeFacade
{
    friend StatesEditorView;

public:
    QmlConnections() = default;

    QmlConnections(const ModelNode &modelNode)
        : QmlModelNodeFacade(modelNode)
    {}

    explicit operator bool() const { return isValid(); }

    bool isValid(SL sl = {}) const;
    static bool isValidQmlConnections(const ModelNode &modelNode, SL sl = {});
    void destroy(SL sl = {});

    QString target(SL sl = {}) const;
    void setTarget(const QString &target, SL sl = {});

    ModelNode getTargetNode(SL sl = {}) const;

    QList<SignalHandlerProperty> signalProperties(SL sl = {}) const;

    static ModelNode createQmlConnections(AbstractView *view, SL sl = {});
};

} //QmlDesigner
