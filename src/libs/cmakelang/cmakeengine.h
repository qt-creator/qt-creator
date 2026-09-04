// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cmakelang.h"
#include "cmakememorypool.h"

#include <QList>
#include <QString>

#include <unordered_set>

namespace CMakeLang {

class CMAKELANG_EXPORT Diagnostic
{
public:
    enum Kind {
        Warning,
        Error
    };

    Kind kind = Error;
    int line = 0;
    int column = 0;
    int position = 0;
    int length = 0;
    QString message;

    bool isError() const { return kind == Error; }
};

class CMAKELANG_EXPORT Engine
{
    Engine(const Engine &other) = delete;
    void operator=(const Engine &other) = delete;

public:
    Engine();
    ~Engine();

    const QString *string(const QString &s);
    const QString *string(QStringView s) { return string(s.toString()); }

    // Takes ownership of the text the tokens and the AST refer to.
    QStringView setSource(const QString &source);

    MemoryPool *pool() { return &_pool; }

    void warning(int line, int column, int position, int length, const QString &message);
    void error(int line, int column, int position, int length, const QString &message);

    const QList<Diagnostic> &diagnostics() const { return _diagnostics; }
    bool hasErrors() const;
    void clearDiagnostics() { _diagnostics.clear(); }

private:
    MemoryPool _pool;
    QString _source;
    std::unordered_set<QString> _strings;
    QList<Diagnostic> _diagnostics;
};

} // namespace CMakeLang
