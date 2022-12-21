// Copyright (C) 2016 BlackBerry Limited. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QStringList>

namespace Qnx::Internal {

class QnxVersionNumber
{
public:
    QnxVersionNumber(const QStringList &segments);
    QnxVersionNumber(const QString &version);
    QnxVersionNumber();

    int size() const;
    bool isEmpty() const;
    QString segment(int index) const;
    QString toString() const;

    static QnxVersionNumber fromTargetName(const QString &targetName);
    static QnxVersionNumber fromFileName(const QString &fileName, const QRegularExpression &regExp);

    bool operator >(const QnxVersionNumber &b) const;

private:
    QStringList m_segments;
};

} // Qnx::Internal
