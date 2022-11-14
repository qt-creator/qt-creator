// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#ifdef WITH_TESTS

#include <QObject>

namespace Core { class IEditor; }
namespace TextEditor { class BaseTextEditor; }

namespace TextEditor::Internal {

class TestProvider;

class CodeAssistTests : public QObject
{
    Q_OBJECT
public:

private slots:
    void initTestCase();

    void testFollowSymbolBigFile();

    void cleanupTestCase();

private:
    TextEditor::BaseTextEditor *m_editor = nullptr;
    QList<Core::IEditor *> m_editorsToClose;
    TestProvider *m_testProvider = nullptr;
};

} // namespace TextEditor::Internal

#endif
