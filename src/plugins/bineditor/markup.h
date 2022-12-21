// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QColor>
#include <QString>
#include <QList>
#include <QMetaType>

namespace BinEditor {

class Markup
{
public:
    Markup(quint64 a = 0, quint64 l = 0, QColor c = Qt::yellow, const QString &tt = QString()) :
        address(a), length(l), color(c), toolTip(tt) {}
    bool covers(quint64 a) const { return a >= address && a < (address + length); }

    quint64 address;
    quint64 length;
    QColor color;
    QString toolTip;
};

} // BinEditor

Q_DECLARE_METATYPE(BinEditor::Markup)
