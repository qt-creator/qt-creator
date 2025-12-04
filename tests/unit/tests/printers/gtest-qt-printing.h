// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtGlobal>

#include <iosfwd>
#include <iterator>
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
class QLatin1String;

template<typename Type, qsizetype Size>
std::ostream &operator<<(std::ostream &out, const QVarLengthArray<Type, Size> &array)
{
    out << "[";
    copy(array.cbegin(), array.cend(), std::ostream_iterator<Type>(out, ", "));
    out << "]";
    return out;
}

template<typename T>
std::ostream &operator<<(std::ostream &out, const QVector<T> &vector)
{
    out << "[";
    copy(vector.cbegin(), vector.cend(), std::ostream_iterator<T>(out, ", "));
    out << "]";
    return out;
}

std::ostream &operator<<(std::ostream &out, const QVariant &QVariant);
std::ostream &operator<<(std::ostream &out, const QString &text);
std::ostream &operator<<(std::ostream &out, QStringView text);
std::ostream &operator<<(std::ostream &out, const QByteArray &byteArray);
std::ostream &operator<<(std::ostream &out, QByteArrayView byteArray);
std::ostream &operator<<(std::ostream &out, QLatin1String text);
std::ostream &operator<<(std::ostream &out, const QTextCharFormat &format);
std::ostream &operator<<(std::ostream &out, const QImage &image);
std::ostream &operator<<(std::ostream &out, const QIcon &icon);
std::ostream &operator<<(std::ostream &out, const QStringList &list);

void PrintTo(const QString &text, std::ostream *os);
void PrintTo(QStringView text, std::ostream *os);
void PrintTo(const QVariant &variant, std::ostream *os);
void PrintTo(const QByteArray &text, std::ostream *os);
void PrintTo(const QByteArray &text, std::ostream *os);
void PrintTo(QByteArrayView text, std::ostream *os);
void PrintTo(QLatin1String text, std::ostream *os);

QT_END_NAMESPACE
