/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

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

public slots:
    void editorModeChanged(Utils::Id newMode, Utils::Id oldMode);
    void openQtc(bool permanent = false);
    void openQds(bool permanent = false);
    void installQds();
    void generateCmake();
    void generateProjectFile();

private:
    static void openInQDSWithProject(const Utils::FilePath &filePath);
    static const QString readFileContents(const Utils::FilePath &filePath);

    bool initialize(const QStringList &arguments, QString *errorString) final;
    void initializeQmlLandingPage();
    void displayQmlLandingPage();
    void hideQmlLandingPage();

    class QmlProjectPluginPrivate *d = nullptr;
};

} // namespace Internal
} // namespace QmlProject
