// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtGlobal>

#include <iosfwd>
#include <ostream>

QT_BEGIN_NAMESPACE

class QVariant;
class QString;
class QStringView;
class QTextCharFormat;
class QImage;
class QIcon;
template<class T, qsizetype Prealloc>
class QVarLengthArray;

template<typename Type, qsizetype Size>
std::ostream &operator<<(std::ostream &out, const QVarLengthArray<Type, Size> &array)
{
    out << "[";

    int i = 0;
    for (auto &&value : array) {
        i++;
        out << value;
        if (i < array.size())
            out << ", ";
    }

    return out << "]";
}

std::ostream &operator<<(std::ostream &out, const QVariant &QVariant);
std::ostream &operator<<(std::ostream &out, const QString &text);
std::ostream &operator<<(std::ostream &out, QStringView text);
std::ostream &operator<<(std::ostream &out, const QByteArray &byteArray);
std::ostream &operator<<(std::ostream &out, QByteArrayView byteArray);
std::ostream &operator<<(std::ostream &out, const QTextCharFormat &format);
std::ostream &operator<<(std::ostream &out, const QImage &image);
std::ostream &operator<<(std::ostream &out, const QIcon &icon);

void PrintTo(const QString &text, std::ostream *os);
void PrintTo(const QVariant &variant, std::ostream *os);
void PrintTo(const QByteArray &text, std::ostream *os);
QT_END_NAMESPACE
