// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "inlinesuppresseddiagnostics.h"

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#ifdef WITH_TESTS
#include <QObject>
#include <QtTest>
#endif // WITH_TESTS

namespace ClangTools::Internal {

InlineSuppressedDiagnostics::~InlineSuppressedDiagnostics() = default;

void InlineSuppressedDiagnostics::addDiagnostic(const QString &diag)
{
    if (!m_diagnostics.contains(diag)) {
        m_diagnostics << diag;
        m_diagnostics.sort();
    }
}

void InlineSuppressedDiagnostics::fromString(const QString &input)
{
    try {
        int tokenOffset = -1;
        for (int i = 0; i < input.size(); ++i) {
            const QChar c = input.at(i);
            if (c.isSpace()) {
                tokenOffset = -1;
                continue;
            }
            if (tokenOffset == -1)
                tokenOffset = i;
            const int tokenPos = i - tokenOffset;
            if (tokenPos >= m_startString.size() || m_startString.at(tokenPos) != input.at(i)) {
                tokenOffset = -1;
                continue;
            }
            if (tokenPos == m_startString.size() - 1) {
                m_parsedStartOffset = tokenOffset;
                fromStringImpl(QStringView(input).mid(m_parsedStartOffset + m_startString.size()));
                break;
            }
        }
    } catch (const ParseError &) {
        m_parsedStartOffset = -1;
        m_parsedEndOffset = -1;
        m_parseError = true;
    }
}

QString InlineSuppressedDiagnostics::toString() const
{
    QTC_ASSERT(!m_diagnostics.isEmpty(), return {});
    if (m_parseError)
        return {};
    return toStringImpl();
}

void InlineSuppressedDiagnostics::parseDiagnostic(QStringView input, int &offset, const QString &prefix)
{
    QString diag;
    while (offset < input.size()) {
        const QChar c = input.at(offset);
        if (!c.isLetterOrNumber() && c != '-')
            break;
        diag.append(c);
        ++offset;
    }
    if (diag.isEmpty())
        throw ParseError();
    m_diagnostics << prefix + diag;
}

void InlineSuppressedClangTidyDiagnostics::fromStringImpl(QStringView input)
{
    int offset = 0;
    const auto skipWhiteSpace = [&] {
        for (; offset < input.size() && input.at(offset).isSpace(); ++offset)
            ;
    };
    while (true) {
        skipWhiteSpace();
        parseDiagnostic(input, offset, {});
        skipWhiteSpace();
        if (offset == input.size())
            throw ParseError();
        const QChar c = input.at(offset);
        if (c == ')') {
            setEndOffset(parsedStartOffset() + startString().size() + offset + 1);
            return;
        }
        if (c == ',') {
            ++offset;
            continue;
        }
        throw ParseError();
    }
}

QString InlineSuppressedClangTidyDiagnostics::toStringImpl() const
{
    return "NOLINT(" + diagnostics().join(',') + ')';
}

void InlineSuppressedClazyDiagnostics::fromStringImpl(QStringView input)
{
    int offset = 0;
    while (true) {
        parseDiagnostic(input, offset, "clazy-");
        if (offset == input.size())
            break;
        const QChar c = input.at(offset);
        if (c == ',') {
            ++offset;
            continue;
        }
        if (c.isSpace())
            break;
        throw ParseError();
    }
    setEndOffset(parsedStartOffset() + startString().size() + offset);
}

QString InlineSuppressedClazyDiagnostics::toStringImpl() const
{
    return "clazy:exclude=" + Utils::transform(diagnostics(), [](const QString &diag) {
                                  return diag.mid(6); // Remove "clazy-" prefix
                              }).join(',');
}

#ifdef WITH_TESTS
class InlineSuppressedDiagnosticsTest : public QObject
{
    Q_OBJECT
private:
    template<typename Diags> void testExcludedDiagnostics()
    {
        QFETCH(QString, input);
        QFETCH(QStringList, additionalDiagnostics);
        QFETCH(int, startOffset);
        QFETCH(int, endOffset);
        QFETCH(QString, output);
        QFETCH(bool, error);

        Diags diags;
        diags.fromString(input);
        for (const QString &diag : std::as_const(additionalDiagnostics))
            diags.addDiagnostic(diag);
        QCOMPARE(diags.parsedStartOffset(), startOffset);
        QCOMPARE(diags.parsedEndOffset(), endOffset);
        QCOMPARE(diags.toString(), output);
        QCOMPARE(diags.hasError(), error);
    }

private slots:
    void testClangTidyExcludedDiagnostics_data()
    {
        QTest::addColumn<QString>("input");
        QTest::addColumn<QStringList>("additionalDiagnostics");
        QTest::addColumn<int>("startOffset");
        QTest::addColumn<int>("endOffset");
        QTest::addColumn<QString>("output");
        QTest::addColumn<bool>("error");

        QTest::newRow("no offset, round-trip")
            << QString("NOLINT(google-explicit-constructor, google-runtime-int)")
            << QStringList() << 0 << 55
            << QString("NOLINT(google-explicit-constructor,google-runtime-int)")
            << false;
        QTest::newRow("random other content, add some diags")
            << QString("some prefix NOLINT(google-runtime-int) some suffix")
            << QStringList{"modernize-concat-nested-namespaces", "misc-include-cleaner"} << 12 << 38
            << QString("NOLINT(google-runtime-int,misc-include-cleaner,modernize-concat-nested-namespaces)")
            << false;
        QTest::newRow("create from empty input")
            << QString()
            << QStringList{"misc-include-cleaner"} << -1 << -1
            << QString("NOLINT(misc-include-cleaner)")
            << false;
        QTest::newRow("create from irrelevant input")
            << QString(" random content ")
            << QStringList{"misc-include-cleaner"} << -1 << -1
            << QString("NOLINT(misc-include-cleaner)")
            << false;
        QTest::newRow("missing closing parenthesis")
            << QString("NOLINT(google-explicit-constructor, google-runtime-int")
            << QStringList() << -1 << -1
            << QString()
            << true;
        QTest::newRow("missing comma")
            << QString("NOLINT(google-explicit-constructor google-runtime-int)")
            << QStringList() << -1 << -1
            << QString()
            << true;
        QTest::newRow("no diagnostics")
            << QString("NOLINT()")
            << QStringList() << -1 << -1
            << QString()
            << true;
        QTest::newRow("empty diagnostic (start)")
            << QString("NOLINT(,google-explicit-constructor,google-runtime-int)")
            << QStringList() << -1 << -1
            << QString()
            << true;
        QTest::newRow("empty diagnostic (middle)")
            << QString("NOLINT(google-explicit-constructor,,google-runtime-int)")
            << QStringList() << -1 << -1
            << QString()
            << true;
        QTest::newRow("empty diagnostic (end)")
            << QString("NOLINT(google-explicit-constructor,)")
            << QStringList() << -1 << -1
            << QString()
            << true;
    }

    void testClangTidyExcludedDiagnostics()
    {
        testExcludedDiagnostics<InlineSuppressedClangTidyDiagnostics>();
    }

    void testClazyExcludedDiagnostics_data()
    {
        QTest::addColumn<QString>("input");
        QTest::addColumn<QStringList>("additionalDiagnostics");
        QTest::addColumn<int>("startOffset");
        QTest::addColumn<int>("endOffset");
        QTest::addColumn<QString>("output");
        QTest::addColumn<bool>("error");

        QTest::newRow("no offset, round-trip")
            << QString("clazy:exclude=qstring-allocations,qenums")
            << QStringList() << 0 << 40
            << QString("clazy:exclude=qstring-allocations,qenums")
            << false;
        QTest::newRow("random other content, add some diags")
            << QString("some prefix clazy:exclude=qstring-allocations some suffix")
            << QStringList{"clazy-qgetenv", "clazy-qtmacros"} << 12 << 45
            << QString("clazy:exclude=qgetenv,qstring-allocations,qtmacros")
            << false;
        QTest::newRow("create from empty input")
            << QString()
            << QStringList{"clazy-qgetenv"} << -1 << -1
            << QString("clazy:exclude=qgetenv")
            << false;
        QTest::newRow("create from irrelevant input")
            << QString(" random content ")
            << QStringList{"clazy-qgetenv"} << -1 << -1
            << QString("clazy:exclude=qgetenv")
            << false;
        QTest::newRow("missing comma")
            << QString("clazy:exclude=qstring-allocations qenums")
            << QStringList() << 0 << 33
            << QString("clazy:exclude=qstring-allocations")
            << false;
        QTest::newRow("empty diagnostic (start)")
            << QString("clazy:exclude=,qenums")
            << QStringList() << -1 << -1
            << QString()
            << true;
        QTest::newRow("empty diagnostic (middle)")
            << QString("clazy:exclude=qstring-allocations,,qenums")
            << QStringList() << -1 << -1
            << QString()
            << true;
        QTest::newRow("empty diagnostic (end)")
            << QString("clazy:exclude=qstring-allocations,qenums,")
            << QStringList() << -1 << -1
            << QString()
            << true;
    }

    void testClazyExcludedDiagnostics()
    {
        testExcludedDiagnostics<InlineSuppressedClazyDiagnostics>();
    }
};

QObject *createInlineSuppressedDiagnosticsTest()
{
    return new InlineSuppressedDiagnosticsTest;
}
#endif // WITH_TESTS

} // namespace ClangTools::Internal

#ifdef WITH_TESTS
#include <inlinesuppresseddiagnostics.moc>
#endif
