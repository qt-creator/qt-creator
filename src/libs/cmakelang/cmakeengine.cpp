// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cmakeengine.h"

#include <algorithm>

using namespace CMakeLang;

Engine::Engine() = default;

Engine::~Engine() = default;

const QString *Engine::string(const QString &s)
{
    return &*_strings.insert(s).first;
}

QStringView Engine::setSource(const QString &source)
{
    _source = source;
    return QStringView(_source);
}

void Engine::warning(int line, int column, int position, int length, const QString &message)
{
    _diagnostics.append({Diagnostic::Warning, line, column, position, length, message});
}

void Engine::error(int line, int column, int position, int length, const QString &message)
{
    _diagnostics.append({Diagnostic::Error, line, column, position, length, message});
}

bool Engine::hasErrors() const
{
    return std::any_of(_diagnostics.cbegin(), _diagnostics.cend(), [](const Diagnostic &d) {
        return d.isError();
    });
}
