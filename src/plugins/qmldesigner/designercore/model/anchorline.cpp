// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "anchorline.h"

namespace QmlDesigner {

AnchorLine::AnchorLine()
    : m_qmlItemNode(QmlItemNode())
    , m_type(AnchorLineInvalid)
{}

AnchorLine::AnchorLine(const QmlItemNode &qmlItemNode, AnchorLineType type)
    : m_qmlItemNode(qmlItemNode),
      m_type(type)
{}

AnchorLineType AnchorLine::type() const
{
    return m_type;
}

bool AnchorLine::isValid() const
{
    return m_type != AnchorLineInvalid && m_qmlItemNode.isValid();
}

bool AnchorLine::isHorizontalAnchorLine(AnchorLineType anchorline)
{
    return anchorline & AnchorLineHorizontalMask;
}

bool AnchorLine::isVerticalAnchorLine(AnchorLineType anchorline)
{
     return anchorline & AnchorLineVerticalMask;
}

QmlItemNode AnchorLine::qmlItemNode() const
{
    return m_qmlItemNode;
}

} // namespace QmlDesigner
