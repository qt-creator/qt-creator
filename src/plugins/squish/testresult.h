// Copyright (C) 2022 The Qt Company Ltd
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QColor>
#include <QMetaType>
#include <QString>

namespace Squish {
namespace Internal {

namespace Result {
enum Type { Log, Pass, Fail, ExpectedFail, UnexpectedPass, Warn, Error, Fatal, Detail, Start, End };
}

class TestResult
{
public:
    TestResult(Result::Type type = Result::Log,
               const QString &text = QString(),
               const QString &timeStamp = QString());

    Result::Type type() const { return m_type; }
    QString text() const { return m_text; }
    QString timeStamp() const { return m_timeStamp; }

    void setText(const QString &text) { m_text = text; }
    void setFile(const QString &file) { m_file = file; }
    void setLine(int line) { m_line = line; }
    QString file() const { return m_file; }
    int line() const { return m_line; }

    static QString typeToString(Result::Type type);
    static QColor colorForType(Result::Type type);

private:
    Result::Type m_type;
    QString m_text;
    QString m_timeStamp;
    QString m_file;
    int m_line;
};

} // namespace Internal
} // namespace Squish

Q_DECLARE_METATYPE(Squish::Internal::TestResult)
Q_DECLARE_METATYPE(Squish::Internal::Result::Type)
