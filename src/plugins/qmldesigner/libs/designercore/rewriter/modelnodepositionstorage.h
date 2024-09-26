// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "modelnode.h"

namespace QmlDesigner {
namespace Internal {

class ModelNodePositionStorage
{
public:
    void clear()
    { m_rewriterData.clear(); }

    int nodeOffset(const ModelNode &modelNode);
    void setNodeOffset(const ModelNode &modelNode, int fileOffset);
    void cleanupInvalidOffsets();
    void removeNodeOffset(const ModelNode &node)
    { m_rewriterData.remove(node); }

    QList<ModelNode> modelNodes() const;

public:
    static const int INVALID_LOCATION = -1;

private:
    class RewriterData { //custom rewriter data for each node can be kept here
    public:
        RewriterData(int offset = INVALID_LOCATION)
            : _offset(offset)
        {}

        bool isValid() const
        { return _offset != INVALID_LOCATION; }

        int offset() const
        { return _offset; }

        void setOffset(int offset)
        { _offset = offset; }

    private:
        int _offset;
    };

private:
    QHash<ModelNode, RewriterData> m_rewriterData;
};

} // namespace Internal
} // namespace QmlDesigner
