// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QString>

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

namespace TextEditor {

class SnippetsSettings
{
public:
    SnippetsSettings() = default;

    void toSettings(const QString &category, QSettings *s) const;
    void fromSettings(const QString &category, QSettings *s);

    void setLastUsedSnippetGroup(const QString &lastUsed);
    const QString &lastUsedSnippetGroup() const;

    bool equals(const SnippetsSettings &snippetsSettings) const;

    friend bool operator==(const SnippetsSettings &a, const SnippetsSettings &b)
    { return a.equals(b); }
    friend bool operator!=(const SnippetsSettings &a, const SnippetsSettings &b)
    { return !a.equals(b); }

private:
    QString m_lastUsedSnippetGroup;
};

} // TextEditor
