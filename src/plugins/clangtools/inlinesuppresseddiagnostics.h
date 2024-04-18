// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QStringList>

QT_BEGIN_NAMESPACE
class QObject;
QT_END_NAMESPACE

namespace ClangTools::Internal {

class InlineSuppressedDiagnostics
{
public:
    virtual ~InlineSuppressedDiagnostics();
    void addDiagnostic(const QString &diag);

    int parsedStartOffset() const { return m_parsedStartOffset; }
    int parsedEndOffset() const { return m_parsedEndOffset; }

    void fromString(const QString &input);
    QString toString() const;

    bool hasError() const { return m_parseError; }

protected:
    class ParseError {};

    InlineSuppressedDiagnostics(const QString &startString) : m_startString(startString) {};

    const QString &startString() const { return m_startString; }
    const QStringList &diagnostics() const { return m_diagnostics; }
    void setEndOffset(int offset) { m_parsedEndOffset = offset; }
    void parseDiagnostic(QStringView input, int &offset, const QString &prefix);

private:
    virtual void fromStringImpl(QStringView input) = 0;
    virtual QString toStringImpl() const = 0;

    const QString m_startString;
    QStringList m_diagnostics;
    int m_parsedStartOffset = -1;
    int m_parsedEndOffset = -1;
    bool m_parseError = false;
};

class InlineSuppressedClangTidyDiagnostics : public InlineSuppressedDiagnostics
{
public:
    InlineSuppressedClangTidyDiagnostics() : InlineSuppressedDiagnostics("NOLINT(") {}

private:
    void fromStringImpl(QStringView input) override;
    QString toStringImpl() const override;
};

class InlineSuppressedClazyDiagnostics : public InlineSuppressedDiagnostics
{
public:
    InlineSuppressedClazyDiagnostics() : InlineSuppressedDiagnostics("clazy:exclude=") {}

private:
    void fromStringImpl(QStringView input) override;
    QString toStringImpl() const override;
};

#ifdef WITH_TESTS
QObject *createInlineSuppressedDiagnosticsTest();
#endif

} // namespace ClangTools::Internal
