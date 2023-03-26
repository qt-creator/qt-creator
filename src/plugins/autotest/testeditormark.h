// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <texteditor/textmark.h>

#include <QPersistentModelIndex>

namespace Autotest {
namespace Internal {

class TestEditorMark : public TextEditor::TextMark
{
public:
    TestEditorMark(QPersistentModelIndex item, const Utils::FilePath &file, int line);

    bool isClickable() const override { return true; }
    void clicked() override;

private:
    QPersistentModelIndex m_item;
};

} // namespace Internal
} // namespace Autotest
