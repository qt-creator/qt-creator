// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "anchorline.h"

#include <qmlitemnode.h>

namespace QmlDesigner {

class QMLDESIGNER_EXPORT QmlAnchors
{
protected:
    using SL = ModelTracing::SourceLocation;

public:
    QmlAnchors(const QmlItemNode &fxItemNode);

    bool isValid(SL sl = {}) const;

    void setAnchor(AnchorLineType sourceAnchorLineType,
                   const QmlItemNode &targetModelNode,
                   AnchorLineType targetAnchorLineType,
                   SL sl = {});
    bool canAnchor(const QmlItemNode &targetModelNode, SL sl = {}) const;
    AnchorLineType possibleAnchorLines(AnchorLineType sourceAnchorLineType,
                                       const QmlItemNode &targetModelNode,
                                       SL sl = {}) const;
    AnchorLine instanceAnchor(AnchorLineType sourceAnchorLineType, SL sl = {}) const;

    void removeAnchor(AnchorLineType sourceAnchorLineType, SL sl = {});
    void removeAnchors(SL sl = {});
    bool instanceHasAnchor(AnchorLineType sourceAnchorLineType, SL sl = {}) const;
    bool instanceHasAnchors(SL sl = {}) const;
    double instanceLeftAnchorLine(SL sl = {}) const;
    double instanceTopAnchorLine(SL sl = {}) const;
    double instanceRightAnchorLine(SL sl = {}) const;
    double instanceBottomAnchorLine(SL sl = {}) const;
    double instanceHorizontalCenterAnchorLine(SL sl = {}) const;
    double instanceVerticalCenterAnchorLine(SL sl = {}) const;
    double instanceAnchorLine(AnchorLineType anchorLine, SL sl = {}) const;

    void setMargin(AnchorLineType sourceAnchorLineType, double margin, SL sl = {}) const;
    bool instanceHasMargin(AnchorLineType sourceAnchorLineType, SL sl = {}) const;
    double instanceMargin(AnchorLineType sourceAnchorLineType, SL sl = {}) const;
    void removeMargin(AnchorLineType sourceAnchorLineType, SL sl = {});
    void removeMargins(SL sl = {});

    void fill(SL sl = {});
    void centerIn(SL sl = {});

    bool checkForCycle(AnchorLineType anchorLineTyp, const QmlItemNode &sourceItem, SL sl = {}) const;
    bool checkForHorizontalCycle(const QmlItemNode &sourceItem, SL sl = {}) const;
    bool checkForVerticalCycle(const QmlItemNode &sourceItem, SL sl = {}) const;

    QmlItemNode qmlItemNode() const;

    bool modelHasAnchors(SL sl = {}) const;
    bool modelHasAnchor(AnchorLineType sourceAnchorLineType, SL sl = {}) const;
    AnchorLine modelAnchor(AnchorLineType sourceAnchorLineType, SL sl = {}) const;

    template<typename String>
    friend void convertToString(String &string, const QmlAnchors &anchors)
    {
        convertToString(string, anchors.m_qmlItemNode);
    }

private:
    QmlItemNode m_qmlItemNode;
};

} //QmlDesigner
