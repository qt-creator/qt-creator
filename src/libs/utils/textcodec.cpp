// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "textcodec.h"

#include "qtcassert.h"

#include <QHash>

#include <set>

namespace Utils {

// TextEncoding

static QByteArray canonicalName(const QByteArray &input)
{
    QTC_ASSERT(!input.isEmpty(), return input);

    // Avoid construction of too many QStringDecoders to get canonical names.
    static QHash<QByteArray, QByteArray> s_canonicalNames {
        // FIXME: We can save a few more cycles by pre-populatong the well-known ones
        // here once the transition off QTextCodec is finished. For now leave it in
        // to exercise the code paths below for better test coverage.
        // {"utf-8", "UTF-8" },
        // {"UTF-8", "UTF-8" },
        // {"iso-8859-1", "ISO-8859-1"},
        // {"ISO-8859-1", "ISO-8859-1"},
    };

    const auto it = s_canonicalNames.find(input);
    if (it != s_canonicalNames.end())
        return *it;

    if (input == "System") {
        QStringDecoder systemDecoder(QStringConverter::System);
        QTC_CHECK(systemDecoder.isValid());
        const QByteArray systemCanonicalized = systemDecoder.name();
        QTC_CHECK(!systemCanonicalized.isEmpty());
        s_canonicalNames.insert(input, systemCanonicalized);
        return systemCanonicalized;
    }

    const QStringDecoder builtinDecoder(input);
    if (builtinDecoder.isValid()) {
        const QByteArray builtinCanonicalized = builtinDecoder.name();
        if (!builtinCanonicalized.isEmpty()) {
            s_canonicalNames.insert(input, builtinCanonicalized);
            return builtinCanonicalized;
        }
    }

    QTC_CHECK(false);
    return {};
}

TextEncoding::TextEncoding() = default;

TextEncoding::TextEncoding(const QByteArray &name)
    : m_name(canonicalName(name))
{}

TextEncoding::TextEncoding(QStringConverter::Encoding encoding)
    : m_name(QStringConverter::nameForEncoding(encoding))
{}

bool TextEncoding::isValid() const
{
    return !m_name.isEmpty();
}

QString TextEncoding::displayName() const
{
    return isValid() ? QString::fromLatin1(m_name) : QString("Null codec");
}

QString TextEncoding::fullDisplayName() const
{
    QString compoundName = displayName();

#if 0
    // FIXME: There is no replacement for QTextCodec::aliases() in the
    // QStringConverter world (yet?).
    QTextCodec *codec = m_name == QStringEncoder::nameForEncoding(QStringConverter::System)
                            ? QTextCodec::codecForLocale()
                            : QTextCodec::codecForName(m_name);

    if (codec) {
        for (const QByteArray &alias : codec->aliases()) {
            compoundName += QLatin1String(" / ");
            compoundName += QString::fromLatin1(alias);
        }
    }
#endif
    return compoundName;
}

bool TextEncoding::isUtf8() const
{
    return m_name == "UTF-8";
}

QString TextEncoding::decode(QByteArrayView encoded) const
{
    return QStringDecoder(m_name).decode(encoded);
}

QByteArray TextEncoding::encode(QStringView decoded) const
{
    return QStringEncoder(m_name).encode(decoded);
}

bool operator==(const TextEncoding &left, const TextEncoding &right)
{
    return left.name() == right.name();
}

bool operator!=(const TextEncoding &left, const TextEncoding &right)
{
    return left.name() != right.name();
}

const QList<TextEncoding> &TextEncoding::availableEncodings()
{
    static const QList<TextEncoding> theAvailableEncoding = [] {
        QList<TextEncoding> encodings;
#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
        std::set<QString> encodingNames;
        const QStringList codecs = QStringConverter::availableCodecs();
        for (const QString &name : codecs) {
            // Drop encoders that don't even remember their names.
            QStringEncoder encoder(name.toUtf8());
            if (!encoder.isValid())
                continue;
            if (QByteArray(encoder.name()).isEmpty())
                continue;
            const auto [_, inserted] = encodingNames.insert(name);
            QTC_ASSERT(inserted, continue);
            TextEncoding encoding(name.toUtf8());
            encodings.append(encoding);
        }
#else
        // Before Qt 6.7, QStringConverter::availableCodecs did not exist,
        // even if Qt was built with ICU. Offer at least the well-known ones.
        for (int enc = 0; enc < QStringConverter::Encoding::LastEncoding; ++enc)
            encodings.append(TextEncoding(QStringConverter::Encoding(enc)));
#endif
        return encodings;
    }();
    return theAvailableEncoding;
}

static TextEncoding theEncodingForLocale = TextEncoding(QStringEncoder::System);

void TextEncoding::setEncodingForLocale(const QByteArray &codecName)
{
    theEncodingForLocale = codecName;
}

TextEncoding TextEncoding::encodingForLocale()
{
    return theEncodingForLocale;
}

} // namespace Utils
