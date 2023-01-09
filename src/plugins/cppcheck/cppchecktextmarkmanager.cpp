// Copyright (C) 2018 Sergey Morozov
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cppchecktextmarkmanager.h"

#include "cppcheckdiagnostic.h"
#include "cppchecktextmark.h"

#include <utils/algorithm.h>

namespace Cppcheck::Internal {

CppcheckTextMarkManager::CppcheckTextMarkManager() = default;

CppcheckTextMarkManager::~CppcheckTextMarkManager() = default;

void CppcheckTextMarkManager::add(const Diagnostic &diagnostic)
{
    std::vector<MarkPtr> &fileMarks = m_marks[diagnostic.fileName];
    if (Utils::contains(fileMarks, [diagnostic](const MarkPtr &mark) {return *mark == diagnostic;}))
        return;

    fileMarks.push_back(std::make_unique<CppcheckTextMark>(diagnostic));
}

void CppcheckTextMarkManager::clearFiles(const Utils::FilePaths &files)
{
    if (m_marks.empty())
        return;

    if (!files.empty()) {
        for (const Utils::FilePath &file : files)
            m_marks.erase(file);
    } else {
        m_marks.clear();
    }
}

} // Cppcheck::Internal
