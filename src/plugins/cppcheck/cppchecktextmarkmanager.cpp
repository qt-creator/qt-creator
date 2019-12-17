/****************************************************************************
**
** Copyright (C) 2018 Sergey Morozov
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "cppcheckdiagnostic.h"
#include "cppchecktextmark.h"
#include "cppchecktextmarkmanager.h"

#include <utils/algorithm.h>

namespace Cppcheck {
namespace Internal {

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

} // namespace Internal
} // namespace Cppcheck
