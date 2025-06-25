// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include <QString>
#include <QStringConverter>

namespace Utils {

class QTCREATOR_UTILS_EXPORT TextEncoding
{
public:
    TextEncoding();
    TextEncoding(const QByteArray &name);
    TextEncoding(QStringEncoder::Encoding encoding);

    static const QStringConverter::Encoding Utf8 = QStringConverter::Encoding::Utf8;
    static const QStringConverter::Encoding Utf16 = QStringConverter::Encoding::Utf16;
    static const QStringConverter::Encoding Utf32 = QStringConverter::Encoding::Utf32;
    static const QStringConverter::Encoding Latin1 = QStringConverter::Encoding::Latin1;
    static const QStringConverter::Encoding System = QStringConverter::Encoding::System;

    bool isValid() const;

    QByteArray name() const { return m_name; }

    QString displayName() const;
    QString fullDisplayName() const;

    bool isUtf8() const;

    QString decode(QByteArrayView encoded) const;
    QByteArray encode(QStringView decoded) const;

    static TextEncoding encodingForLocale();
    static void setEncodingForLocale(const QByteArray &codecName);

    static QList<QByteArray> availableCodecs();

    // FIXME: Avoid. Not present in QStringConverter.
    static TextEncoding encodingForMib(int mib);
    static QList<int> availableMibs();

private:
    QTCREATOR_UTILS_EXPORT friend bool operator==(const TextEncoding &left, const TextEncoding &right);
    QTCREATOR_UTILS_EXPORT friend bool operator!=(const TextEncoding &left, const TextEncoding &right);

    QByteArray m_name;
};

} // namespace Utils
