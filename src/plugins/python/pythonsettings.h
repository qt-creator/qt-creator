// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/kitaspect.h>
#include <projectexplorer/runconfigurationaspects.h>

#include <solutions/tasking/tasktreerunner.h>

#include <utils/filepath.h>
#include <utils/listmodel.h>

namespace Python::Internal {

class PythonSettings : public QObject
{
    Q_OBJECT

public:
    PythonSettings();
    ~PythonSettings();

    using Interpreter = ProjectExplorer::Interpreter;

    static QList<Interpreter> interpreters();
    static Interpreter defaultInterpreter();
    static Interpreter interpreter(const QString &interpreterId);
    static void setInterpreter(const QList<Interpreter> &interpreters, const QString &defaultId);
    static Interpreter createInterpreter(
        const Utils::FilePath &python,
        const QString &defaultName,
        const QString &suffix = {},
        const ProjectExplorer::DetectionSource &detectionSource = {});

    static void addInterpreter(const Interpreter &interpreter, bool isDefault = false);
    static Interpreter addInterpreter(const Utils::FilePath &interpreterPath,
                                      bool isDefault = false,
                                      const QString &nameSuffix = {});
    static void setPyLSConfiguration(const QString &configuration);
    static bool pylsEnabled();
    static void setPylsEnabled(const bool &enabled);
    static QString pylsConfiguration();
    static PythonSettings *instance();
    static void createVirtualEnvironmentInteractive(
        const Utils::FilePath &startDirectory,
        const Interpreter &defaultInterpreter,
        const std::function<void(const Utils::FilePath &)> &callback);
    static void createVirtualEnvironment(
        const Utils::FilePath &interpreter,
        const Utils::FilePath &directory,
        const std::function<void(const Utils::FilePath &)> &callback = {});
    static QList<Interpreter> detectPythonVenvs(const Utils::FilePath &path);
    static void addKitsForInterpreter(const Interpreter &interpreter, bool force);
    static void removeKitsForInterpreter(const Interpreter &interpreter);
    static bool interpreterIsValid(const Interpreter &interpreter);

    static std::optional<Tasking::ExecutableItem> autoDetect(
        ProjectExplorer::Kit *kit,
        const Utils::FilePaths &searchPaths,
        const QString &detectionSource,
        const ProjectExplorer::LogCallback &logCallback);
    static void removeDetectedPython(
        const QString &detectionSource, const ProjectExplorer::LogCallback &logCallback);
    static void listDetectedPython(
        const QString &detectionSource, const ProjectExplorer::LogCallback &logCallback);

signals:
    void interpretersChanged(const QList<Interpreter> &interpreters, const QString &defaultId);
    void pylsConfigurationChanged(const QString &configuration);
    void pylsEnabledChanged(const bool enabled);
    void virtualEnvironmentCreated(const Utils::FilePath &venvPython);

private:
    void disableOutdatedPyls();
    void disableOutdatedPylsNow();
    void fixupPythonKits();
    void initFromSettings(Utils::QtcSettings *settings);
    void writeToSettings(Utils::QtcSettings *settings);

    QList<Interpreter> m_interpreters;
    QString m_defaultInterpreterId;
    bool m_pylsEnabled = true;
    QString m_pylsConfiguration;
    Tasking::TaskTreeRunner m_taskTreeRunner;

    static void saveSettings();
};

void setupPythonSettings();

Utils::ListModel<ProjectExplorer::Interpreter> *createInterpreterModel(QObject *parent);

} // Python::Internal
