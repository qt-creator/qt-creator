// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "texteditor_global.h"

#include <QVariantMap>

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

namespace TextEditor {

class TEXTEDITOR_EXPORT StorageSettings
{
public:
    StorageSettings();

    void toSettings(const QString &category, QSettings *s) const;
    void fromSettings(const QString &category, QSettings *s);

    QVariantMap toMap() const;
    void fromMap(const QVariantMap &map);

    // calculated based on boolean setting plus file type blacklist examination
    bool removeTrailingWhitespace(const QString &filePattern) const;

    bool equals(const StorageSettings &ts) const;
    friend bool operator==(const StorageSettings &t1, const StorageSettings &t2)
    { return t1.equals(t2); }
    friend bool operator!=(const StorageSettings &t1, const StorageSettings &t2)
    { return !t1.equals(t2); }

    QString m_ignoreFileTypes;
    bool m_cleanWhitespace;
    bool m_inEntireDocument;
    bool m_addFinalNewLine;
    bool m_cleanIndentation;
    bool m_skipTrailingWhitespace;
};

} // namespace TextEditor
