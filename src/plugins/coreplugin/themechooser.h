// Copyright (C) 2016 Thorben Kroeger <thorbenkroeger@gmail.com>.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/id.h>

#include <QWidget>

namespace Utils { class Theme; }

namespace Core::Internal {

class ThemeEntry final
{
public:
    ThemeEntry() = default;
    ThemeEntry(Utils::Id id, const QString &filePath);

    Utils::Id id() const;
    QString displayName() const;
    QString filePath() const;
    static QList<ThemeEntry> availableThemes();
    static Utils::Id themeSetting();
    static Utils::Theme *createTheme(Utils::Id id);

private:
    Utils::Id m_id;
    QString m_filePath;
    mutable QString m_displayName;
};

class ThemeChooser final : public QWidget
{
public:
    ThemeChooser();
    ~ThemeChooser() final;

    void apply();

private:
    class ThemeChooserPrivate *d;
};

} // namespace Core::Internal
