// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/projectmanager.h>
#include <qmlprojectmanager/qmlprojectmanager_global.h>
#include <utils/filepath.h>
#include <utils/qtcprocess.h>

namespace QmlProjectManager::QmlProjectExporter {

class QMLPROJECTMANAGER_EXPORT ResourceGenerator : public QObject
{
    Q_OBJECT
public:
    ResourceGenerator(QObject *parent = nullptr);
    static void generateMenuEntry(QObject *parent);

    Q_INVOKABLE static bool createQrc(const ProjectExplorer::Project *project,
                                      const Utils::FilePath &qrcFilePath);
    Q_INVOKABLE static std::optional<Utils::FilePath> createQrc(const ProjectExplorer::Project *project);

    Q_INVOKABLE bool createQmlrc(const ProjectExplorer::Project *project,
                                 const Utils::FilePath &qmlrcFilePath);
    Q_INVOKABLE std::optional<Utils::FilePath> createQmlrc(const ProjectExplorer::Project *project);

    Q_INVOKABLE void createQmlrcAsync(const ProjectExplorer::Project *project);
    Q_INVOKABLE void createQmlrcAsync(const ProjectExplorer::Project *project,
                                      const Utils::FilePath &qmlrcFilePath);

    Q_INVOKABLE void cancel();

private:
    Utils::Process m_rccProcess;
    Utils::FilePath m_qmlrcFilePath;

private:
    bool runRcc(const Utils::FilePath &qmlrcFilePath,
                const Utils::FilePath &qrcFilePath,
                const bool runAsync = false);

signals:
    void errorOccurred(const QString &error);
    void qmlrcCreated(const Utils::FilePath &filePath);
};

} // namespace QmlProjectManager::QmlProjectExporter
