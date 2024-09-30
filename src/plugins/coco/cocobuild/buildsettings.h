#pragma once

#include <QObject>
#include <QStringList>

// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

namespace ProjectExplorer {
class BuildConfiguration;
class Project;
class Target;
}

namespace Coco::Internal {

class CocoBuildStep;
class CocoProjectWidget;
class ModificationFile;

class BuildSettings : public QObject
{
    Q_OBJECT
public:
    static bool supportsBuildConfig(const ProjectExplorer::BuildConfiguration &config);
    static BuildSettings *createdFor(const ProjectExplorer::BuildConfiguration &config);

    explicit BuildSettings(ModificationFile &featureFile, ProjectExplorer::Project *project);
    virtual ~BuildSettings() {}

    void connectToBuildStep(CocoBuildStep *step) const;
    virtual void connectToProject(CocoProjectWidget *) const {}
    virtual void read() = 0;
    bool enabled() const;
    virtual bool validSettings() const = 0;
    virtual void setCoverage(bool on) = 0;

    virtual QString saveButtonText() const = 0;
    virtual void reconfigure() {};
    virtual void stopReconfigure() {};
    virtual bool needsReconfigure() const { return false; }

    virtual QString configChanges() const = 0;

    const QStringList &options() const;
    const QStringList &tweaks() const;
    virtual QString projectDirectory() const = 0;

    bool hasTweaks() const;
    QString featureFilenName() const;
    QString featureFilePath() const;

    virtual void write(const QString &options, const QString &tweaks) = 0;
    void provideFile();

protected:
    QString tableRow(const QString &name, const QString &value) const;
    void setEnabled(bool enabled);
    ProjectExplorer::Target *activeTarget() const;

private:
    ModificationFile &m_featureFile;
    ProjectExplorer::Project &m_project;
    bool m_enabled = false;
};

} // namespace Coco::Internal
