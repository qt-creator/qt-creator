// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <qmldesignercorelib_global.h>
#include <modelnode.h>

namespace QmlDesigner {

class AbstractView;
class NodeInstanceView;

class QMLDESIGNERCORE_EXPORT QmlModelNodeFacade
{
public:
    operator ModelNode() const;
    ModelNode modelNode() const { return m_modelNode; }
    bool hasModelNode() const;
    static bool isValidQmlModelNodeFacade(const ModelNode &modelNode);
    virtual bool isValid() const;
    virtual ~QmlModelNodeFacade();
    QmlModelNodeFacade();

    AbstractView *view() const;
    Model *model() const;
    static NodeInstanceView *nodeInstanceView(const ModelNode &modelNode);
    NodeInstanceView *nodeInstanceView() const;
    bool isRootNode() const;

protected:
    QmlModelNodeFacade(const ModelNode &modelNode);

private:
    ModelNode m_modelNode;
};

} //QmlDesigner
