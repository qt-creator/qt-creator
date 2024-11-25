// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include <cppeditor/clangdiagnosticconfig.h>
#include <utils/filepath.h>

#include <QObject>

namespace ClangTools::Internal {
class ClangToolsCompilationDb : public QObject
{
    Q_OBJECT
public:
    ~ClangToolsCompilationDb();

    bool generateIfNecessary();
    Utils::FilePath parentDir() const;

    static ClangToolsCompilationDb &getDb(CppEditor::ClangToolType toolType);

signals:
    void generated(bool success);

private:
    explicit ClangToolsCompilationDb(CppEditor::ClangToolType toolType);
    void invalidate();

    class Private;
    Private * const d;
};

} // namespace ClangTools::Internal
