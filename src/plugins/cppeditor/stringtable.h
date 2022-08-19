// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <QString>

namespace CppEditor::Internal {

class StringTable
{
public:
    static QString insert(const QString &string);
    static void scheduleGC();

private:
    friend class CppEditorPluginPrivate;
    StringTable();
    ~StringTable();
};

} // CppEditor::Internal
