// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <extensionsystem/iplugin.h>
#include <coreplugin/dialogs/ioptionspage.h>
#include <utils/pathchooser.h>

#include <QTimer>

QT_FORWARD_DECLARE_CLASS(QCheckBox)

namespace StudioWelcome {
namespace Internal {

class StudioSettingsPage : public Core::IOptionsPageWidget
{
    Q_OBJECT

public:
    void apply() final;

    StudioSettingsPage();

private:
    QCheckBox *m_buildCheckBox;
    QCheckBox *m_debugCheckBox;
    QCheckBox *m_analyzeCheckBox;
    Utils::PathChooser *m_pathChooser;
};

class StudioWelcomeSettingsPage : public Core::IOptionsPage
{
    Q_OBJECT

public:
    StudioWelcomeSettingsPage();
};

class StudioWelcomePlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "StudioWelcome.json")

public slots:
    void closeSplashScreen();

public:
    StudioWelcomePlugin();
    ~StudioWelcomePlugin() final;

    bool initialize(const QStringList &arguments, QString *errorString) override;
    void extensionsInitialized() override;
    bool delayedInitialize() override;

    static Utils::FilePath defaultExamplesPath();
    static QString examplesPathSetting();

signals:
    void examplesDownloadPathChanged(const QString &path);

private:
    class WelcomeMode *m_welcomeMode = nullptr;
    StudioWelcomeSettingsPage m_settingsPage;
};

} // namespace Internal
} // namespace StudioWelcome
