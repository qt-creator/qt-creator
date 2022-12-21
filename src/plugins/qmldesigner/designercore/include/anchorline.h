// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qmldesignercorelib_global.h>

#include "qmlitemnode.h"

namespace QmlDesigner {

class QMLDESIGNERCORE_EXPORT AnchorLine
{
public:
    AnchorLine();
    AnchorLine(const QmlItemNode &qmlItemNode, AnchorLineType type);
    AnchorLineType type() const;
    bool isValid() const;

    static bool isHorizontalAnchorLine(AnchorLineType anchorline);
    static bool isVerticalAnchorLine(AnchorLineType anchorline);

    QmlItemNode qmlItemNode() const;

private:
    QmlItemNode m_qmlItemNode;
    AnchorLineType m_type;
};

} // namespace QmlDesigner
