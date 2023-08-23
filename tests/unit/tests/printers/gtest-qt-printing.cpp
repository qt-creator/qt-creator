// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QDebug>
#include <QIcon>
#include <QString>
#include <QTextCharFormat>

#include <gtest/gtest-printers.h>

#include <iostream>

QT_BEGIN_NAMESPACE

std::ostream &operator<<(std::ostream &out, const QByteArray &byteArray)
{
    if (byteArray.contains('\n')) {
        QByteArray formattedArray = byteArray;
        formattedArray.replace("\n", "\n\t");
        out << "\n\t";
        out.write(formattedArray.data(), formattedArray.size());
    } else {
        out << "\"";
        out.write(byteArray.data(), byteArray.size());
        out << "\"";
    }

    return out;
}

std::ostream &operator<<(std::ostream &out, const QString &text)
{
    return out << text.toUtf8();
}

std::ostream &operator<<(std::ostream &out, QStringView text)
{
    return out << text.toString();
}

std::ostream &operator<<(std::ostream &out, const QVariant &variant)
{
    QString output;
    QDebug debug(&output);

    debug.noquote().nospace() << variant;

    QByteArray utf8Text = output.toUtf8();

    return out.write(utf8Text.data(), utf8Text.size());
}

std::ostream &operator<<(std::ostream &out, const QTextCharFormat &format)
{
    out << "("
        << format.fontFamily();

    if (format.fontItalic())
        out << ", italic";

    if (format.fontCapitalization() == QFont::Capitalize)
        out << ", Capitalization";

    if (format.fontOverline())
        out << ", overline";

    out << ")";

    return out;
}

std::ostream &operator<<(std::ostream &out, const QImage &image)
{
    return out << "(" << image.width() << ", " << image.height() << ", " << image.format() << ")";
}

std::ostream &operator<<(std::ostream &out, const QIcon &icon)
{
    return out << "(";

    for (const QSize &size : icon.availableSizes())
        out << "(" << size.width() << ", " << size.height() << "), ";

    out << icon.cacheKey() << ")";
}

void PrintTo(const QString &text, std::ostream *os)
{
    *os << text;
}

void PrintTo(const QVariant &variant, std::ostream *os)
{
    *os << variant;
}

void PrintTo(const QByteArray &text, std::ostream *os)
{
    *os << text;
}

QT_END_NAMESPACE
