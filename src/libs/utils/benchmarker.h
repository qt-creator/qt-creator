// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include <QString>
#include <QElapsedTimer>

QT_BEGIN_NAMESPACE
class QLoggingCategory;
QT_END_NAMESPACE

namespace Utils {

class QTCREATOR_UTILS_EXPORT Benchmarker
{
public:
    Benchmarker(const QString &testsuite, const QString &testcase,
                const QString &tagData = QString());
    Benchmarker(const QLoggingCategory &cat, const QString &testsuite, const QString &testcase,
                const QString &tagData = QString());
    ~Benchmarker();

    void report(qint64 ms);
    static void report(const QString &testsuite, const QString &testcase,
                       qint64 ms, const QString &tags = QString());
    static void report(const QLoggingCategory &cat,
                       const QString &testsuite, const QString &testcase,
                       qint64 ms, const QString &tags = QString());

private:
    const QLoggingCategory &m_category;
    QElapsedTimer m_timer;
    QString m_tagData;
    QString m_testsuite;
    QString m_testcase;
};

} // namespace Utils
