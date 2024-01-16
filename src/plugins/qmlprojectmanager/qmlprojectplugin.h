// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/editormanager/editormanager.h>
#include <extensionsystem/iplugin.h>
#include <utils/filepath.h>

namespace QmlProjectManager::Internal {

void openQDS(const Utils::FilePath &fileName);
Utils::FilePath qdsInstallationEntry();
bool qdsInstallationExists();
bool checkIfEditorIsuiQml(Core::IEditor *editor);
Utils::FilePath projectFilePath();
Utils::FilePaths rootCmakeFiles();
QString qtVersion(const Utils::FilePath &projectFilePath);
QString qdsVersion(const Utils::FilePath &projectFilePath);
void openInQDSWithProject(const Utils::FilePath &filePath);
const QString readFileContents(const Utils::FilePath &filePath);

class QmlProjectPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "QmlProjectManager.json")

public:
    QmlProjectPlugin() = default;
    ~QmlProjectPlugin() final;

public slots:
    void editorModeChanged(Utils::Id newMode, Utils::Id oldMode);
    void openQtc(bool permanent = false);
    void openQds(bool permanent = false);

private:
    void initialize() final;
    void displayQmlLandingPage();
    void hideQmlLandingPage();
    void updateQmlLandingPageProjectInfo(const Utils::FilePath &projectFile);

    class QmlProjectPluginPrivate *d = nullptr;
};

} // QmlProject::Internal
