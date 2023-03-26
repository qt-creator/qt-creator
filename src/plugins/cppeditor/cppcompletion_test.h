// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

namespace CppEditor::Internal {

class CompletionTest : public QObject
{
    Q_OBJECT

private slots:
    void testCompletionBasic1();

    void testCompletionTemplateFunction_data();
    void testCompletionTemplateFunction();

    void testCompletion_data();
    void testCompletion();

    void testGlobalCompletion_data();
    void testGlobalCompletion();

    void testDoxygenTagCompletion_data();
    void testDoxygenTagCompletion();

    void testCompletionMemberAccessOperator_data();
    void testCompletionMemberAccessOperator();

    void testCompletionPrefixFirstQTCREATORBUG_8737();
    void testCompletionPrefixFirstQTCREATORBUG_9236();
};

} // namespace CppEditor::Internal
