// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cppindexingsupport.h"

namespace CppEditor {

CppIndexingSupport::~CppIndexingSupport() = default;

SymbolSearcher::SymbolSearcher(QObject *parent)
    : QObject(parent)
{
}

SymbolSearcher::~SymbolSearcher() = default;

} // namespace CppEditor
