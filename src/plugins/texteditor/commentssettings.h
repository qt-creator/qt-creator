// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "texteditor_global.h"

#include <coreplugin/dialogs/ioptionspage.h>

namespace ProjectExplorer { class Project; }

namespace TextEditor {

class TEXTEDITOR_EXPORT CommentsSettings
{
public:
    enum class CommandPrefix { Auto, At, Backslash };
    class Data {
    public:
        CommandPrefix commandPrefix = CommandPrefix::Auto;
        bool enableDoxygen = true;
        bool generateBrief = true;
        bool leadingAsterisks = true;
    };

    static Data data() { return instance().m_data; }
    static void setData(const Data &data);

    static Utils::Key mainSettingsKey();
    static Utils::Key enableDoxygenSettingsKey();
    static Utils::Key generateBriefSettingsKey();
    static Utils::Key leadingAsterisksSettingsKey();
    static Utils::Key commandPrefixKey();

private:
    CommentsSettings();
    static CommentsSettings &instance();
    void save() const;
    void load();

    Data m_data;
};
inline bool operator==(const CommentsSettings::Data &a, const CommentsSettings::Data &b)
{
    return a.enableDoxygen == b.enableDoxygen
           && a.commandPrefix == b.commandPrefix
           && a.generateBrief == b.generateBrief
           && a.leadingAsterisks == b.leadingAsterisks;
}
inline bool operator!=(const CommentsSettings::Data &a, const CommentsSettings::Data &b)
{
    return !(a == b);
}


class TEXTEDITOR_EXPORT CommentsSettingsWidget final : public Core::IOptionsPageWidget
{
    Q_OBJECT
public:
    CommentsSettingsWidget(const CommentsSettings::Data &settings);
    ~CommentsSettingsWidget();

    CommentsSettings::Data settingsData() const;

signals:
    void settingsChanged();

private:
    void apply() override;

    void initFromSettings(const CommentsSettings::Data &settings);

    class Private;
    Private * const d;
};

namespace Internal {

class CommentsSettingsPage : public Core::IOptionsPage
{
public:
    CommentsSettingsPage();
};

} // namespace Internal
} // namespace TextEditor
