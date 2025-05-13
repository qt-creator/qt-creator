// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "textcodec.h"

#include "qtcassert.h"

#include <QTextCodec>

namespace Utils {

TextCodec::TextCodec() = default;

TextCodec::TextCodec(QTextCodec *codec)
    : m_codec(codec)
{}

QByteArray TextCodec::name() const
{
    return m_codec ? m_codec->name() : QByteArray();
}

QString TextCodec::displayName() const
{
    return m_codec ? QString::fromLatin1(m_codec->name()) : QString("Null codec");
}

bool TextCodec::isValid() const
{
    return m_codec;
}

TextCodec TextCodec::codecForLocale()
{
    if (QTextCodec *codec = QTextCodec::codecForLocale())
        return TextCodec(codec);
    QTC_CHECK(false);
    return {};
}

bool TextCodec::isUtf8() const
{
    return m_codec && m_codec->name() == "UTF-8";
}

TextCodec TextCodec::utf8()
{
    static TextCodec theUtf8Codec(QTextCodec::codecForName("UTF-8"));
    return theUtf8Codec;
}

TextCodec TextCodec::utf16()
{
    static TextCodec theUtf16Codec(QTextCodec::codecForName("UTF-16"));
    return theUtf16Codec;
}

TextCodec TextCodec::utf32()
{
    static TextCodec theUtf32Codec(QTextCodec::codecForName("UTF-32"));
    return theUtf32Codec;
}

TextCodec TextCodec::latin1()
{
    static TextCodec theLatin1Codec(QTextCodec::codecForName("latin1"));
    return theLatin1Codec;
}

QTextCodec *TextCodec::asQTextCodec() const
{
    return m_codec;
}

QByteArray TextCodec::fromUnicode(QStringView data) const
{
    if (m_codec)
        return m_codec->fromUnicode(data);

    QTC_CHECK(false);
    return {};
}

QString TextCodec::toUnicode(const QByteArray &data) const
{
    if (m_codec)
        return m_codec->toUnicode(data);

    QTC_CHECK(false);
    return {};
}

QString TextCodec::toUnicode(QByteArrayView data) const
{
    if (m_codec)
        return m_codec->toUnicode(data.constData(), data.size());

    QTC_CHECK(false);
    return {};
}

TextCodec TextCodec::codecForName(const QByteArray &codecName)
{
    return TextCodec(QTextCodec::codecForName(codecName));
}

bool operator==(const TextCodec &left, const TextCodec &right)
{
    return left.m_codec == right.m_codec;
}

} // namespace Utils
