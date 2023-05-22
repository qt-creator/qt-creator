// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "modelfwd.h"
#include "qmldesignercorelib_exports.h"

#include <QtGlobal>
#include <QList>

#include <nodeinstanceglobal.h>
#include <projectstorage/projectstoragefwd.h>

namespace QmlDesigner {

enum AnchorLineType {
    AnchorLineInvalid = 0x0,
    AnchorLineNoAnchor = AnchorLineInvalid,
    AnchorLineLeft = 0x01,
    AnchorLineRight = 0x02,
    AnchorLineTop = 0x04,
    AnchorLineBottom = 0x08,
    AnchorLineHorizontalCenter = 0x10,
    AnchorLineVerticalCenter = 0x20,
    AnchorLineBaseline = 0x40,

    AnchorLineFill = AnchorLineLeft | AnchorLineRight | AnchorLineTop | AnchorLineBottom,
    AnchorLineCenter = AnchorLineVerticalCenter | AnchorLineHorizontalCenter,
    AnchorLineHorizontalMask = AnchorLineLeft | AnchorLineRight | AnchorLineHorizontalCenter,
    AnchorLineVerticalMask = AnchorLineTop | AnchorLineBottom | AnchorLineVerticalCenter
                             | AnchorLineBaseline,
    AnchorLineAllMask = AnchorLineVerticalMask | AnchorLineHorizontalMask
};

} // namespace QmlDesigner
