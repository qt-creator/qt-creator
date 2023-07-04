// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qmldesignercorelib_global.h>

#include <modelnode.h>
#include <nodemetainfo.h>

namespace QmlDesigner {

class AbstractView;
class NodeInstanceView;

class QMLDESIGNERCORE_EXPORT QmlModelNodeFacade
{
public:
    operator ModelNode() const { return m_modelNode; }
    ModelNode modelNode() const { return m_modelNode; }
    bool hasModelNode() const;
    static bool isValidQmlModelNodeFacade(const ModelNode &modelNode);
    bool isValid() const;
    explicit operator bool() const { return isValid(); }
    QmlModelNodeFacade() = default;

    AbstractView *view() const;
    Model *model() const;
    NodeMetaInfo metaInfo() const { return m_modelNode.metaInfo(); }
    static const NodeInstanceView *nodeInstanceView(const ModelNode &modelNode);
    const NodeInstanceView *nodeInstanceView() const;
    bool isRootNode() const;

    static void enableUglyWorkaroundForIsValidQmlModelNodeFacadeInTests();

    friend bool operator==(const QmlModelNodeFacade &firstNode, const QmlModelNodeFacade &secondNode)
    {
        return firstNode.m_modelNode == secondNode.m_modelNode;
    }

    friend bool operator==(const QmlModelNodeFacade &firstNode, const ModelNode &secondNode)
    {
        return firstNode.m_modelNode == secondNode;
    }

    friend bool operator==(const ModelNode &firstNode, const QmlModelNodeFacade &secondNode)
    {
        return firstNode == secondNode.m_modelNode;
    }

    friend bool operator!=(const QmlModelNodeFacade &firstNode, const QmlModelNodeFacade &secondNode)
    {
        return !(firstNode == secondNode);
    }

    friend bool operator!=(const QmlModelNodeFacade &firstNode, const ModelNode &secondNode)
    {
        return firstNode.m_modelNode != secondNode;
    }

    friend bool operator!=(const ModelNode &firstNode, const QmlModelNodeFacade &secondNode)
    {
        return firstNode != secondNode.m_modelNode;
    }

    friend bool operator<(const QmlModelNodeFacade &firstNode, const QmlModelNodeFacade &secondNode)
    {
        return firstNode.m_modelNode < secondNode.m_modelNode;
    }

protected:
    QmlModelNodeFacade(const ModelNode &modelNode)
        : m_modelNode(modelNode)
    {}

private:
    ModelNode m_modelNode;
};

} //QmlDesigner
