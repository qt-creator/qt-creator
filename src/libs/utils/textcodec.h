// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include <QString>
#include <QStringConverter>

QT_BEGIN_NAMESPACE
class QTextCodec;
QT_END_NAMESPACE

namespace Utils {

class QTCREATOR_UTILS_EXPORT TextEncoding
{
public:
    TextEncoding();
    TextEncoding(const QByteArray &name);
    TextEncoding(QStringEncoder::Encoding encoding);

    operator QByteArray() const { return m_name; }
    QByteArray name() const { return m_name; }

    bool isValid() const;

private:
    QTCREATOR_UTILS_EXPORT friend bool operator==(const TextEncoding &left, const TextEncoding &right);
    QTCREATOR_UTILS_EXPORT friend bool operator!=(const TextEncoding &left, const TextEncoding &right);

    QByteArray m_name;
};

class QTCREATOR_UTILS_EXPORT TextCodec final
{
public:
    using ConverterState = QStringConverter::State;

    TextCodec();

    bool isValid() const;

    int mibEnum() const;
    QByteArray name() const;
    QString displayName() const;
    QString fullDisplayName() const; // Includes aliases

    QByteArray fromUnicode(QStringView data) const;

    QString toUnicode(const QByteArray &data) const;
    QString toUnicode(QByteArrayView data) const;
    QString toUnicode(const char *data, int size, ConverterState *state) const;

    bool canEncode(QStringView data) const;

    static TextCodec codecForName(const QByteArray &codecName);
    static TextCodec codecForMib(int mib);
    static TextCodec codecForLocale();

    bool isUtf8() const;
    static bool isUtf8Codec(const QByteArray &codecName); // Also considers aliases

    static QList<int> availableMibs();
    static QList<QByteArray> availableCodecs();

    static TextCodec utf8();
    static TextCodec utf16();
    static TextCodec utf32();
    static TextCodec latin1();

    static void setCodecForLocale(const QByteArray &codecName);
    static TextEncoding encodingForLocale();

private:
    explicit TextCodec(QTextCodec *codec);

    QTCREATOR_UTILS_EXPORT friend bool operator==(const TextCodec &left, const TextCodec &right);

    QTextCodec *m_codec = nullptr; // FIXME: Avoid later
};

} // namespace Utils
