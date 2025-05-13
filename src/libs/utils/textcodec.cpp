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

QString TextCodec::fullDisplayName() const
{
    QString compoundName = displayName();
    if (m_codec) {
        for (const QByteArray &alias : m_codec->aliases()) {
            compoundName += QLatin1String(" / ");
            compoundName += QString::fromLatin1(alias);
        }
    }
    return compoundName;
}

bool TextCodec::isValid() const
{
    return m_codec;
}

int TextCodec::mibEnum() const
{
    return m_codec ? m_codec->mibEnum() : -1;
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

bool TextCodec::isUtf8Codec(const QByteArray &name)
{
    static const auto utf8Codecs = []() -> QList<QByteArray> {
        const TextCodec codec = TextCodec::utf8();
        if (QTC_GUARD(codec.isValid()))
            return QList<QByteArray>{codec.name()} + codec.m_codec->aliases();
        return {"UTF-8"};
    }();

    return utf8Codecs.contains(name);
}

QList<int> TextCodec::availableMibs()
{
    return QTextCodec::availableMibs();
}

QList<QByteArray> TextCodec::availableCodecs()
{
    return QTextCodec::availableCodecs();
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

void TextCodec::setCodecForLocale(const QByteArray &codecName)
{
    QTextCodec::setCodecForLocale(QTextCodec::codecForName(codecName));
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

QString TextCodec::toUnicode(const char *data, int size, ConverterState *state) const
{
    if (m_codec)
        return m_codec->toUnicode(data, size, state);

    QTC_CHECK(false);
    return {};
}

bool TextCodec::canEncode(QStringView data) const
{
    return m_codec && m_codec->canEncode(data);
}

TextCodec TextCodec::codecForName(const QByteArray &codecName)
{
    return TextCodec(QTextCodec::codecForName(codecName));
}

TextCodec TextCodec::codecForMib(int mib)
{
    return TextCodec(QTextCodec::codecForMib(mib));
}

bool operator==(const TextCodec &left, const TextCodec &right)
{
    return left.m_codec == right.m_codec;
}

} // namespace Utils
