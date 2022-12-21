// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtGlobal>
#include <QList>

#include <utils/smallstringview.h>

#include <nodeinstanceglobal.h>

// Unnecessary since core isn't a dll any more.

#if defined(QMLDESIGNERCORE_LIBRARY)
#define QMLDESIGNERCORE_EXPORT Q_DECL_EXPORT
#elif defined(QMLDESIGNERCORE_STATIC_LIBRARY)
#define QMLDESIGNERCORE_EXPORT
#else
#define QMLDESIGNERCORE_EXPORT Q_DECL_IMPORT
#endif

namespace QmlDesigner {
using PropertyName = QByteArray;
using PropertyNameList = QList<PropertyName>;
using TypeName = QByteArray;
using PropertyTypeList = QList<PropertyName>;
using IdName = QByteArray;

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

    AnchorLineFill =  AnchorLineLeft | AnchorLineRight | AnchorLineTop | AnchorLineBottom,
    AnchorLineCenter = AnchorLineVerticalCenter | AnchorLineHorizontalCenter,
    AnchorLineHorizontalMask = AnchorLineLeft | AnchorLineRight | AnchorLineHorizontalCenter,
    AnchorLineVerticalMask = AnchorLineTop | AnchorLineBottom | AnchorLineVerticalCenter | AnchorLineBaseline,
    AnchorLineAllMask = AnchorLineVerticalMask | AnchorLineHorizontalMask
};

struct ModelDeleter
{
    QMLDESIGNERCORE_EXPORT void operator()(class Model *model);
};

using ModelPointer = std::unique_ptr<class Model, ModelDeleter>;

constexpr bool useProjectStorage()
{
#ifdef QDS_USE_PROJECTSTORAGE
    return true;
#else
    return false;
#endif
}
} // namespace QmlDesigner
