// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

namespace CppEditor::Internal {

class SourceProcessorTest : public QObject
{
    Q_OBJECT

private slots:
    void testIncludesResolvedUnresolved();
    void testIncludesCyclic();
    void testIncludesAllDiagnostics();
    void testMacroUses();
    void testIncludeNext();
};

} // namespace CppEditor::Internal
