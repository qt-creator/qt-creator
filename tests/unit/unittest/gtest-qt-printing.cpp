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

#include <QString>
#include <QDebug>

#include <gtest/gtest-printers.h>

#include <iostream>

QT_BEGIN_NAMESPACE

void PrintTo(const QByteArray &byteArray, ::std::ostream *os)
{
    if (byteArray.contains('\n')) {
        QByteArray formattedArray = byteArray;
        formattedArray.replace("\n", "\n\t");
        *os << "\n\t";
        os->write(formattedArray.data(), formattedArray.size());
    } else {
        *os << "\"";
        os->write(byteArray.data(), byteArray.size());
        *os << "\"";
    }
}

void PrintTo(const QVariant &variant, ::std::ostream *os)
{
    QString output;
    QDebug debug(&output);

    debug << variant;

    PrintTo(output.toUtf8(), os);
}

void PrintTo(const QString &text, ::std::ostream *os)
{
    const QByteArray utf8 = text.toUtf8();

    PrintTo(text.toUtf8(), os);
}

QT_END_NAMESPACE
