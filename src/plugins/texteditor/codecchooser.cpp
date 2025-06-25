// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "codecchooser.h"

#include <utils/algorithm.h>

using namespace Utils;

namespace TextEditor {

CodecChooser::CodecChooser()
{
    for (const TextEncoding &encoding : TextEncoding::availableEncodings()) {
        addItem(encoding.fullDisplayName());
        m_encodings.append(encoding);
    }
    connect(this, &QComboBox::currentIndexChanged,
            this, [this](int index) { emit encodingChanged(encodingAt(index)); });
}

void CodecChooser::prependNone()
{
    insertItem(0, "None");
    m_encodings.prepend({});
}

TextEncoding CodecChooser::currentEncoding() const
{
    return encodingAt(currentIndex());
}

TextEncoding CodecChooser::encodingAt(int index) const
{
    if (index < 0)
        index = 0;
    return m_encodings[index].isValid() ? m_encodings[index] : TextEncoding();
}

void CodecChooser::setAssignedEncoding(const TextEncoding &encoding, const QString &name)
{
    int rememberedSystemPosition = -1;
    for (int i = 0, total = m_encodings.size(); i < total; ++i) {
        if (encoding != m_encodings.at(i))
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
            : m_encodings.at(index).name();
}

} // TextEditor
