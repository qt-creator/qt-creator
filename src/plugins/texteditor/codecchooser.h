// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "texteditor_global.h"

#include <utils/textcodec.h>

#include <QComboBox>

namespace TextEditor {

class TEXTEDITOR_EXPORT CodecChooser : public QComboBox
{
    Q_OBJECT

public:
    CodecChooser();

    void prependNone();
    Utils::TextEncoding currentEncoding() const;
    void setAssignedEncoding(const Utils::TextEncoding &encoding, const QString &name = {});
    QByteArray assignedCodecName() const;

signals:
    void encodingChanged(const Utils::TextEncoding &encoding);

private:
    Utils::TextEncoding encodingAt(int index) const;
    QList<Utils::TextEncoding> m_encodings;
};

} // namespace TextEditor
