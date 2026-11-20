// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QByteArray>
#include <QString>

#include <cstdio>

int main()
{
    QString qstringLocal = QString::fromLatin1("0123456789");
    QString literalLocal = QStringLiteral("literal");
    QByteArray qbytearrayLocal("0123456789");
    QString &qstringReference = qstringLocal;
    QByteArray *qbytearrayPointer = &qbytearrayLocal;
    std::printf("%d %d %d\n", int(qstringReference.size()), int(literalLocal.size()),
                int(qbytearrayPointer->size())); // breakpoint line
    return 0;
}
