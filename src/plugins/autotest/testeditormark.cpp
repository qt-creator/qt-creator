// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "testeditormark.h"

#include "autotesttr.h"
#include "testresultspane.h"

namespace Autotest {
namespace Internal {

TestEditorMark::TestEditorMark(QPersistentModelIndex item, const Utils::FilePath &file, int line)
    : TextEditor::TextMark(file, line, {Tr::tr("Auto Test"), Utils::Id(Constants::TASK_MARK_ID)}),
      m_item(item)
{
}

void TestEditorMark::clicked()
{
    auto instance = TestResultsPane::instance();
    instance->showTestResult(m_item);
}

} // namespace Internal
} // namespace Autotest
