// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cppsemanticinfo.h"

namespace CppEditor::Internal {

class LocalSymbols
{
    Q_DISABLE_COPY(LocalSymbols)

public:
    LocalSymbols(CPlusPlus::Document::Ptr doc, CPlusPlus::DeclarationAST *ast);

    SemanticInfo::LocalUseMap uses;
};

} // namespace CppEditor::Internal
