// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <modelnode.h>
#include <qmldesignercorelib_global.h>

namespace QmlDesigner {

class AbstractView;
class NodeInstanceView;

class QmlModelNodeFacade
{
public:
    operator ModelNode() const { return {}; }
    ModelNode modelNode() { return {}; }
    const ModelNode modelNode() const { return {}; }
    bool hasModelNode() const { return {}; }
    static bool isValidQmlModelNodeFacade([[maybe_unused]] const ModelNode &modelNode)
    {
        return {};
    }
    virtual bool isValid() const { return {}; }

    AbstractView *view() const { return {}; }
    static NodeInstanceView *nodeInstanceView([[maybe_unused]] const ModelNode &modelNode)
    {
        return {};
    }
    NodeInstanceView *nodeInstanceView() const { return {}; }
    bool isRootNode() const { return {}; }

    QmlModelNodeFacade(const ModelNode &) {}
    QmlModelNodeFacade() {}
    ~QmlModelNodeFacade(){};
};

} // namespace QmlDesigner
