// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <cppeditor/cppquickfix.h>

namespace ClangTools {
namespace Internal {

class DocumentClangToolRunner;

class DocumentQuickFixFactory : public CppEditor::CppQuickFixFactory
{
public:
    using RunnerCollector = std::function<DocumentClangToolRunner *(const Utils::FilePath &)>;

    DocumentQuickFixFactory(RunnerCollector runnerCollector);
    void match(const CppEditor::Internal::CppQuickFixInterface &interface,
               QuickFixOperations &result) override;

private:
    RunnerCollector m_runnerCollector;
};

} // namespace Internal
} // namespace ClangTools
