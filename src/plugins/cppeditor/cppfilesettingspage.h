// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cppeditorconstants.h"

#include <coreplugin/dialogs/ioptionspage.h>
#include <projectexplorer/projectsettingswidget.h>

#include <QDir>

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

namespace ProjectExplorer { class Project; }

namespace CppEditor::Internal {

class CppFileSettings
{
public:
    QStringList headerPrefixes;
    QString headerSuffix = "h";
    QStringList headerSearchPaths = {"include",
                                     "Include",
                                     QDir::toNativeSeparators("../include"),
                                     QDir::toNativeSeparators("../Include")};
    QStringList sourcePrefixes;
    QString sourceSuffix = "cpp";
    QStringList sourceSearchPaths = {QDir::toNativeSeparators("../src"),
                                     QDir::toNativeSeparators("../Src"),
                                     ".."};
    QString licenseTemplatePath;
    bool headerPragmaOnce = false;
    bool lowerCaseFiles = Constants::LOWERCASE_CPPFILES_DEFAULT;

    void toSettings(Utils::QtcSettings *) const;
    void fromSettings(Utils::QtcSettings *);
    void addMimeInitializer() const;
    bool applySuffixesToMimeDB();

    // Convenience to return a license template completely formatted.
    QString licenseTemplate() const;

    bool equals(const CppFileSettings &rhs) const;
    bool operator==(const CppFileSettings &s) const { return equals(s); }
    bool operator!=(const CppFileSettings &s) const { return !equals(s); }
};

class CppFileSettingsForProject
{
public:
    CppFileSettingsForProject(ProjectExplorer::Project *project);

    CppFileSettings settings() const;
    void setSettings(const CppFileSettings &settings);
    bool useGlobalSettings() const { return m_useGlobalSettings; }
    void setUseGlobalSettings(bool useGlobal);

private:
    void loadSettings();
    void saveSettings();

    ProjectExplorer::Project * const m_project;
    CppFileSettings m_customSettings;
    bool m_useGlobalSettings = true;
};

class CppFileSettingsPage : public Core::IOptionsPage
{
public:
    explicit CppFileSettingsPage(CppFileSettings *settings);
};

class CppFileSettingsForProjectWidget : public ProjectExplorer::ProjectSettingsWidget
{
public:
    CppFileSettingsForProjectWidget(const CppFileSettingsForProject &settings);
    ~CppFileSettingsForProjectWidget();

private:
    class Private;
    Private * const d;
};

} // namespace CppEditor::Internal
