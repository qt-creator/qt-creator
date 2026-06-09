// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/dialogs/ioptionspage.h>
#include <utils/aspects.h>

namespace ExtensionSystem { class IPlugin; }
namespace ProjectExplorer { class Project; }

namespace CppEditor::Internal {

class CommaSeparatedStringsAspect final : public Utils::StringAspect
{
public:
    explicit CommaSeparatedStringsAspect(Utils::AspectContainer *container);

    QStringList operator()() const;

    void push(const QString &item);
    void pop();
};

class SuffixSelectionAspect final : public Utils::StringSelectionAspect
{
public:
    explicit SuffixSelectionAspect(Utils::AspectContainer *container);

    void setMimeType(const QString &mimeType);
    void fixupComboBox(QComboBox *comboBox) final;
};

class CppFileSettings : public Utils::AspectContainer
{
public:
    CppFileSettings();

    SuffixSelectionAspect headerSuffix{this};
    CommaSeparatedStringsAspect headerPrefixes{this};
    CommaSeparatedStringsAspect headerSearchPaths{this};

    SuffixSelectionAspect sourceSuffix{this};
    CommaSeparatedStringsAspect sourcePrefixes{this};
    CommaSeparatedStringsAspect sourceSearchPaths{this};

    Utils::FilePathAspect licenseTemplatePath{this};
    Utils::StringAspect headerGuardTemplate{this};

    Utils::BoolAspect headerPragmaOnce{this};
    Utils::BoolAspect lowerCaseFiles{this};

    void addMimeInitializer() const;
    bool applySuffixesToMimeDB();

    // Convenience to return a license template completely formatted.
    QString licenseTemplate() const;

    // Expanded headerGuardTemplate.
    QString headerGuard(const Utils::FilePath &headerFilePath) const;

private:
    void apply() final;
    void slotEdit();
};

class CppFileSettingsData
{
public:
    QString headerSuffix;
    QStringList headerPrefixes;
    QStringList headerSearchPaths;

    QString sourceSuffix;
    QStringList sourcePrefixes;
    QStringList sourceSearchPaths;

    Utils::FilePath licenseTemplatePath;
    QString headerGuardTemplate;

    bool headerPragmaOnce = false;
    bool lowerCaseFiles = false;
};

CppFileSettingsData cppFileSettingsForProject(ProjectExplorer::Project *project);
QString headerGuardForProject(ProjectExplorer::Project *project, const Utils::FilePath &headerFilePath);
QString licenseTemplateForProject(ProjectExplorer::Project *project);

CppFileSettings &globalCppFileSettings();

void setupCppFileSettings(ExtensionSystem::IPlugin &plugin);

} // namespace CppEditor::Internal
