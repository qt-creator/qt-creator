// Copyright (C) 2018 Sergey Morozov
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <cppcheck/cppcheckdiagnosticmanager.h>

#include <utils/filepath.h>

#include <unordered_map>

namespace Cppcheck::Internal {

class Diagnostic;
class CppcheckTextMark;

class CppcheckTextMarkManager final : public CppcheckDiagnosticManager
{
public:
    explicit CppcheckTextMarkManager();
    ~CppcheckTextMarkManager() override;

    void add(const Diagnostic &diagnostic) override;
    void clearFiles(const Utils::FilePaths &files);

private:
    using MarkPtr = std::unique_ptr<CppcheckTextMark>;
    std::unordered_map<Utils::FilePath, std::vector<MarkPtr>> m_marks;
};

} // Cppcheck::Internal
