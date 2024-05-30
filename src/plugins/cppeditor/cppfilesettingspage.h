// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cppeditorconstants.h"

#include <coreplugin/dialogs/ioptionspage.h>

#include <QDir>

namespace ExtensionSystem { class IPlugin; }
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
    Utils::FilePath licenseTemplatePath;
    QString headerGuardTemplate = "%{JS: '%{Header:FileName}'.toUpperCase().replace(/[.]/g, '_')}";
    bool headerPragmaOnce = false;
    bool lowerCaseFiles = Constants::LOWERCASE_CPPFILES_DEFAULT;

    void toSettings(Utils::QtcSettings *) const;
    void fromSettings(Utils::QtcSettings *);
    void addMimeInitializer() const;
    bool applySuffixesToMimeDB();

    // Convenience to return a license template completely formatted.
    QString licenseTemplate() const;

    // Expanded headerGuardTemplate.
    QString headerGuard(const Utils::FilePath &headerFilePath) const;

    bool equals(const CppFileSettings &rhs) const;
    bool operator==(const CppFileSettings &s) const { return equals(s); }
    bool operator!=(const CppFileSettings &s) const { return !equals(s); }
};

CppFileSettings &globalCppFileSettings();

CppFileSettings cppFileSettingsForProject(ProjectExplorer::Project *project);

void setupCppFileSettings(ExtensionSystem::IPlugin &plugin);

} // namespace CppEditor::Internal
