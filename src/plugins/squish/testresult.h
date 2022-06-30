/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator Squish plugin.
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
