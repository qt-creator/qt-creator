// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "codecchooser.h"

#include <utils/algorithm.h>

#include <QTextCodec>

static bool isSingleByte(int mib)
{
    // Encodings are listed at https://www.iana.org/assignments/character-sets/character-sets.xhtml
    return (mib >= 0 && mib <= 16)
            || (mib >= 81 && mib <= 85)
            || (mib >= 109 && mib <= 112)
            || (mib >= 2000 && mib <= 2024)
            || (mib >= 2028 && mib <= 2100)
            || (mib >= 2106);
}

namespace TextEditor {

CodecChooser::CodecChooser(Filter filter)
{
    QList<int> mibs = Utils::sorted(QTextCodec::availableMibs());
    QList<int>::iterator firstNonNegative =
        std::find_if(mibs.begin(), mibs.end(), [](int n) { return n >=0; });
    if (firstNonNegative != mibs.end())
        std::rotate(mibs.begin(), firstNonNegative, mibs.end());
    for (int mib : std::as_const(mibs)) {
        if (filter == Filter::SingleByte && !isSingleByte(mib))
            continue;
        if (QTextCodec *codec = QTextCodec::codecForMib(mib)) {
            QString compoundName = QLatin1String(codec->name());
            const QList<QByteArray> aliases = codec->aliases();
            for (const QByteArray &alias : aliases) {
                compoundName += QLatin1String(" / ");
                compoundName += QString::fromLatin1(alias);
            }
            addItem(compoundName);
            m_codecs.append(codec);
        }
    }
    connect(this, &QComboBox::currentIndexChanged,
            this, [this](int index) { emit codecChanged(m_codecs.at(index)); });
}

void CodecChooser::prependNone()
{
    insertItem(0, "None");
    m_codecs.prepend(nullptr);
}

QTextCodec *CodecChooser::currentCodec() const
{
    return codecAt(currentIndex());
}

QTextCodec *CodecChooser::codecAt(int index) const
{
    if (index < 0)
        index = 0;
    return m_codecs[index];
}

void CodecChooser::setAssignedCodec(QTextCodec *codec, const QString &name)
{
    int rememberedSystemPosition = -1;
    for (int i = 0, total = m_codecs.size(); i < total; ++i) {
        if (codec != m_codecs.at(i))
            continue;
        if (name.isEmpty() || itemText(i) == name) {
            setCurrentIndex(i);
            return;
        }
        // we've got System matching encoding - but have explicitly set the codec
        rememberedSystemPosition = i;
    }
    if (rememberedSystemPosition != -1)
        setCurrentIndex(rememberedSystemPosition);
}

QByteArray CodecChooser::assignedCodecName() const
{
    const int index = currentIndex();
    return index == 0
            ? QByteArray("System")   // we prepend System to the available codecs
            : m_codecs.at(index)->name();
}

} // TextEditor
