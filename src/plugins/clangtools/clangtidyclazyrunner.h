// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <cppeditor/clangdiagnosticconfig.h>

#include "clangtoolrunner.h"

namespace ClangTools {
namespace Internal {

class ClangTidyRunner final : public ClangToolRunner
{
    Q_OBJECT

public:
    ClangTidyRunner(const CppEditor::ClangDiagnosticConfig &config, QObject *parent = nullptr);
};

class ClazyStandaloneRunner final : public ClangToolRunner
{
    Q_OBJECT

public:
    ClazyStandaloneRunner(const CppEditor::ClangDiagnosticConfig &config, QObject *parent = nullptr);
};

} // namespace Internal
} // namespace ClangTools
