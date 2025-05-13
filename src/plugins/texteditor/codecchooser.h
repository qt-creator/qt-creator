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
    enum class Filter { All, SingleByte };

    explicit CodecChooser(Filter filter = Filter::All);

    void prependNone();
    Utils::TextCodec currentCodec() const;
    void setAssignedCodec(const Utils::TextCodec &codec, const QString &name = {});
    QByteArray assignedCodecName() const;

signals:
    void codecChanged(const Utils::TextCodec &codec);

private:
    Utils::TextCodec codecAt(int index) const;
    QList<Utils::TextCodec> m_codecs;
};

} // namespace TextEditor
