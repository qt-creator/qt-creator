// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "texteditor_global.h"

#include <QComboBox>

QT_BEGIN_NAMESPACE
class QTextCodec;
QT_END_NAMESPACE

namespace TextEditor {

class TEXTEDITOR_EXPORT CodecChooser : public QComboBox
{
    Q_OBJECT

public:
    enum class Filter { All, SingleByte };
    explicit CodecChooser(Filter filter = Filter::All);
    void prependNone();
    QTextCodec *currentCodec() const;
    QTextCodec *codecAt(int index) const;
    void setAssignedCodec(QTextCodec *codec, const QString &name = {});
    QByteArray assignedCodecName() const;

signals:
    void codecChanged(QTextCodec *codec);

private:
    QList<QTextCodec *> m_codecs;
};

} // namespace TextEditor
