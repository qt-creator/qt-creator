// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "codecchooser.h"

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

void CodecChooser::setAssignedEncoding(const TextEncoding &encoding)
{
    if (const qsizetype index = m_encodings.indexOf(encoding); index >= 0)
        setCurrentIndex(index);
}

} // TextEditor
