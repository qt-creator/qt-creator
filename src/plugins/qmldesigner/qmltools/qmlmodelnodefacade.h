// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qmldesigner_global.h>

#include <modelnode.h>
#include <nodemetainfo.h>

namespace QmlDesigner {

class AbstractView;
class NodeInstanceView;

class QMLDESIGNER_EXPORT QmlModelNodeFacade
{
protected:
    using SL = ModelTracing::SourceLocation;

public:
    operator ModelNode() const { return m_modelNode; }

    const ModelNode &modelNode() const { return m_modelNode; }

    ModelNode &modelNode() { return m_modelNode; }

    bool hasModelNode() const;
    static bool isValidQmlModelNodeFacade(const ModelNode &modelNode, SL sl = {});
    bool isValid(SL sl = {}) const;
    explicit operator bool() const { return isValid(); }
    QmlModelNodeFacade() = default;

    AbstractView *view() const;
    Model *model() const;
    NodeMetaInfo metaInfo() const { return m_modelNode.metaInfo(); }
    static const NodeInstanceView *nodeInstanceView(const ModelNode &modelNode);
    const NodeInstanceView *nodeInstanceView() const;
    bool isRootNode(SL sl = {}) const;

    bool operator==(const QmlModelNodeFacade &other) const
    {
        return m_modelNode == other.m_modelNode;
    }

    bool operator==(const ModelNode &other) const { return m_modelNode == other; }

    auto operator<=>(const QmlModelNodeFacade &other) const
    {
        return m_modelNode <=> other.m_modelNode;
    }

    template<typename String>
    friend void convertToString(String &string, const QmlModelNodeFacade &node)
    {
        convertToString(string, node.m_modelNode);
    }

protected:
    QmlModelNodeFacade(const ModelNode &modelNode)
        : m_modelNode(modelNode)
    {}

private:
    ModelNode m_modelNode;
};

QMLDESIGNER_EXPORT const NodeInstanceView *nodeInstanceView(const ModelNode &node);

} //QmlDesigner
