/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include <clangbackendipc_global.h>

#include <clang-c/Index.h>

#include <functional>
#include <vector>

class Utf8String;

namespace ClangBackEnd {

class SourceLocation;
class SourceRange;
class FixIt;
class DiagnosticSet;
class DiagnosticContainer;
class SourceRangeContainer;
class FixItContainer;

class Diagnostic
{
    friend class DiagnosticSet;
    friend class DiagnosticSetIterator;
    friend bool operator==(Diagnostic first, Diagnostic second);

public:
    ~Diagnostic();

    Diagnostic(const Diagnostic &) = delete;
    const Diagnostic &operator=(const Diagnostic &) = delete;

    Diagnostic(Diagnostic &&other);
    Diagnostic &operator=(Diagnostic &&other);

    bool isNull() const;

    Utf8String text() const;
    Utf8String category() const;
    std::pair<Utf8String, Utf8String> options() const;
    SourceLocation location() const;
    DiagnosticSeverity severity() const;
    std::vector<SourceRange> ranges() const;
    std::vector<FixIt> fixIts() const;
    DiagnosticSet childDiagnostics() const;

    DiagnosticContainer toDiagnosticContainer() const;

private:
    Diagnostic(CXDiagnostic cxDiagnostic);
    QVector<SourceRangeContainer> getSourceRangeContainers() const;
    QVector<FixItContainer> getFixItContainers() const;

private:
    CXDiagnostic cxDiagnostic;
};

inline bool operator==(Diagnostic first, Diagnostic second)
{
    return first.cxDiagnostic == second.cxDiagnostic;
}

} // namespace ClangBackEnd
