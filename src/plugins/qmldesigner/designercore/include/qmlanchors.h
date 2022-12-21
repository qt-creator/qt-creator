// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qmldesignercorelib_global.h>
#include <qmlitemnode.h>
#include "anchorline.h"

namespace QmlDesigner {

class QMLDESIGNERCORE_EXPORT QmlAnchors
{
public:
    QmlAnchors(const QmlItemNode &fxItemNode);

    bool isValid() const;

    void setAnchor(AnchorLineType sourceAnchorLineType,
                   const QmlItemNode &targetModelNode,
                   AnchorLineType targetAnchorLineType);
    bool canAnchor(const QmlItemNode &targetModelNode) const;
    AnchorLineType possibleAnchorLines(AnchorLineType sourceAnchorLineType,
                                         const QmlItemNode &targetModelNode) const;
    AnchorLine instanceAnchor(AnchorLineType sourceAnchorLineType) const;

    void removeAnchor(AnchorLineType sourceAnchorLineType);
    void removeAnchors();
    bool instanceHasAnchor(AnchorLineType sourceAnchorLineType) const;
    bool instanceHasAnchors() const;
    double instanceLeftAnchorLine() const;
    double instanceTopAnchorLine() const;
    double instanceRightAnchorLine() const;
    double instanceBottomAnchorLine() const;
    double instanceHorizontalCenterAnchorLine() const;
    double instanceVerticalCenterAnchorLine() const;
    double instanceAnchorLine(AnchorLineType anchorLine) const;

    void setMargin(AnchorLineType sourceAnchorLineType, double margin) const;
    bool instanceHasMargin(AnchorLineType sourceAnchorLineType) const;
    double instanceMargin(AnchorLineType sourceAnchorLineType) const;
    void removeMargin(AnchorLineType sourceAnchorLineType);
    void removeMargins();

    void fill();
    void centerIn();

    bool checkForCycle(AnchorLineType anchorLineTyp, const QmlItemNode &sourceItem) const;
    bool checkForHorizontalCycle(const QmlItemNode &sourceItem) const;
    bool checkForVerticalCycle(const QmlItemNode &sourceItem) const;

    QmlItemNode qmlItemNode() const;

    bool modelHasAnchors() const;
    bool modelHasAnchor(AnchorLineType sourceAnchorLineType) const;
    AnchorLine modelAnchor(AnchorLineType sourceAnchorLineType) const;


private:
    QmlItemNode m_qmlItemNode;
};

} //QmlDesigner
