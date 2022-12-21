// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "texteditor_global.h"

#include <QVariantMap>

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

namespace TextEditor {

class TEXTEDITOR_EXPORT ExtraEncodingSettings
{
public:
    ExtraEncodingSettings();
    ~ExtraEncodingSettings();

    void toSettings(const QString &category, QSettings *s) const;
    void fromSettings(const QString &category, QSettings *s);

    QVariantMap toMap() const;
    void fromMap(const QVariantMap &map);

    bool equals(const ExtraEncodingSettings &s) const;

    friend bool operator==(const ExtraEncodingSettings &a, const ExtraEncodingSettings &b)
    { return a.equals(b); }

    friend bool operator!=(const ExtraEncodingSettings &a, const ExtraEncodingSettings &b)
    { return !a.equals(b); }

    static QStringList lineTerminationModeNames();

    enum Utf8BomSetting {
        AlwaysAdd = 0,
        OnlyKeep = 1,
        AlwaysDelete = 2
    };
    Utf8BomSetting m_utf8BomSetting;
};

} // TextEditor
