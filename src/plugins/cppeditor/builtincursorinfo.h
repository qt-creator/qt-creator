// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cppcursorinfo.h"
#include "cppeditor_global.h"

#include <cplusplus/CppDocument.h>

#include <QFuture>

namespace CppEditor {

class CPPEDITOR_EXPORT BuiltinCursorInfo
{
public:
    static QFuture<CursorInfo> run(const CursorInfoParams &params);

    static SemanticInfo::LocalUseMap
    findLocalUses(const CPlusPlus::Document::Ptr &document, int line, int column);
};

} // namespace CppEditor
