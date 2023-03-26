// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

namespace CppEditor::Internal {

class SymbolSearcherTest : public QObject
{
    Q_OBJECT

private slots:
    void test();
    void test_data();
};

} // namespace CppEditor::Internal
