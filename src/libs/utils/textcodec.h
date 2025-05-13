// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include <QString>

QT_BEGIN_NAMESPACE
class QTextCodec;
QT_END_NAMESPACE

namespace Utils {

class QTCREATOR_UTILS_EXPORT TextCodec final
{
public:
    TextCodec();

    bool isValid() const;
    QByteArray name() const;

    QByteArray fromUnicode(QStringView data) const;
    QString toUnicode(const QByteArray &data) const;
    QString toUnicode(QByteArrayView data) const;

    static TextCodec codecForName(const QByteArray &codecName);
    static TextCodec codecForLocale();

    bool isUtf8() const;

    static TextCodec utf8();
    static TextCodec utf16();
    static TextCodec utf32();
    static TextCodec latin1();

    QTextCodec *asQTextCodec() const; // FIXME: Avoid.

private:
    explicit TextCodec(QTextCodec *codec);

    QTCREATOR_UTILS_EXPORT friend bool operator==(const TextCodec &left, const TextCodec &right);

    QTextCodec *m_codec = nullptr; // FIXME: Avoid later
};

} // namespace Utils
