// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cppeditor_global.h"

#include <utils/filepath.h>
#include <utils/id.h>
#include <utils/store.h>
#include <utils/qtcsettings.h>

#include <QObject>
#include <QStringList>

namespace ProjectExplorer { class Project; }

namespace CppEditor {
enum class UsePrecompiledHeaders;

class CPPEDITOR_EXPORT CppCodeModelSettings : public QObject
{
    Q_OBJECT

public:
    enum PCHUsage {
        PchUse_None = 1,
        PchUse_BuildSystem = 2
    };

    class Data
    {
    public:
        Data() = default;
        Data(const Utils::Store &store) { fromMap(store); }
        Utils::Store toMap() const;
        void fromMap(const Utils::Store &store);

        friend bool operator==(const Data &s1, const Data &s2);
        friend bool operator!=(const Data &s1, const Data &s2) { return !(s1 == s2); }

        PCHUsage pchUsage = PchUse_BuildSystem;
        bool interpretAmbigiousHeadersAsC = false;
        bool skipIndexingBigFiles = true;
        bool useBuiltinPreprocessor = true;
        int indexerFileSizeLimitInMb = 5;
        bool categorizeFindReferences = false; // Ephemeral!
        bool ignoreFiles = false;
        QString ignorePattern;
    };

    CppCodeModelSettings(const Data &data) : m_data(data) {}

    static CppCodeModelSettings &globalInstance(); // TODO: Make inaccessible.
    static CppCodeModelSettings settingsForProject(const ProjectExplorer::Project *project);
    static CppCodeModelSettings settingsForProject(const Utils::FilePath &projectFile);
    static CppCodeModelSettings settingsForFile(const Utils::FilePath &file);

    static void setGlobalData(const Data &data); // TODO: Make inaccessible.
    void setData(const Data &data) { m_data = data; }
    Data data() const { return m_data; }

    PCHUsage pchUsage() const { return m_data.pchUsage; }
    static PCHUsage pchUsage(const ProjectExplorer::Project *project);
    UsePrecompiledHeaders usePrecompiledHeaders() const;
    static UsePrecompiledHeaders usePrecompiledHeaders(const ProjectExplorer::Project *project);

    bool interpretAmbigiousHeadersAsC() const { return m_data.interpretAmbigiousHeadersAsC; }
    bool skipIndexingBigFiles() const { return m_data.skipIndexingBigFiles; }
    bool useBuiltinPreprocessor() const { return m_data.useBuiltinPreprocessor; }
    int indexerFileSizeLimitInMb() const { return m_data.indexerFileSizeLimitInMb; }
    int effectiveIndexerFileSizeLimitInMb() const;
    bool ignoreFiles() const { return m_data.ignoreFiles; }
    QString ignorePattern() const { return m_data.ignorePattern; }

    static bool categorizeFindReferences();
    static void setCategorizeFindReferences(bool categorize);

signals:
    void changed(ProjectExplorer::Project *project);

private:
    CppCodeModelSettings() = default;
    CppCodeModelSettings(Utils::QtcSettings *s) { fromSettings(s); }

    void toSettings(Utils::QtcSettings *s);
    void fromSettings(Utils::QtcSettings *s);

    Data m_data;
};

class CppCodeModelProjectSettings
{
public:
    CppCodeModelProjectSettings(ProjectExplorer::Project *project);

    CppCodeModelSettings::Data data() const;
    void setData(const CppCodeModelSettings::Data &data);
    bool useGlobalSettings() const { return m_useGlobalSettings; }
    void setUseGlobalSettings(bool useGlobal);

private:
    void loadSettings();
    void saveSettings();

    ProjectExplorer::Project * const m_project;
    CppCodeModelSettings::Data m_customSettings;
    bool m_useGlobalSettings = true;
};

} // namespace CppEditor
