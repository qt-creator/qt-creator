// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

namespace TextEditor { class BaseTextEditor; }

namespace TextEditor::Internal {

class GenerigHighlighterTests : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase();
    void testHighlight_data();
    void testHighlight();
    void testChange();
    void testPreeditText();
    void cleanupTestCase();

private:
    TextEditor::BaseTextEditor *m_editor = nullptr;
};

} // namespace TextEditor::Internal
