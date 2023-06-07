// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmldesignercorelib_global.h"

namespace QmlDesigner {

class QMLDESIGNERCORE_EXPORT PropertyNode // : public BaseModelNode
{
public:
    PropertyNode();

//    static int variantTypeId() { return qMetaTypeId<InternalNode::Pointer>(); }
//    static QVariant toVariant(const InternalNode::Pointer &internalNodePointer) { return QVariant::fromValue(internalNodePointer); }
};

} // namespace QmlDesigner
