// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

namespace CppEditor::Internal {

class PointerDeclarationFormatterTest : public QObject
{
    Q_OBJECT

private slots:
    void testInSimpledeclarations();
    void testInSimpledeclarations_data();
    void testInControlflowstatements();
    void testInControlflowstatements_data();
    void testMultipleDeclarators();
    void testMultipleDeclarators_data();
    void testMultipleMatches();
    void testMultipleMatches_data();
    void testMacros();
    void testMacros_data();
};

} // namespace CppEditor::Internal
