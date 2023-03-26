// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/editormanager/editormanager.h>
#include <extensionsystem/iplugin.h>
#include <utils/filepath.h>

namespace QmlProjectManager {
namespace Internal {

class QmlProjectPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "QmlProjectManager.json")

public:
    QmlProjectPlugin() = default;
    ~QmlProjectPlugin() final;

    static void openQDS(const Utils::FilePath &fileName);
    static Utils::FilePath qdsInstallationEntry();
    static bool qdsInstallationExists();
    static bool checkIfEditorIsuiQml(Core::IEditor *editor);
    static Utils::FilePath projectFilePath();
    static Utils::FilePaths rootCmakeFiles();
    static QString qtVersion(const Utils::FilePath &projectFilePath);
    static QString qdsVersion(const Utils::FilePath &projectFilePath);
    static void openInQDSWithProject(const Utils::FilePath &filePath);
    static const QString readFileContents(const Utils::FilePath &filePath);

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

} // namespace Internal
} // namespace QmlProject
