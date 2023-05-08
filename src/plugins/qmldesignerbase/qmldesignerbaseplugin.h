// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "qmldesignerbase_global.h"

#include <coreplugin/dialogs/ioptionspage.h>
#include <extensionsystem/iplugin.h>
#include <utils/pathchooser.h>

#include <memory>

QT_FORWARD_DECLARE_CLASS(QCheckBox)

namespace QmlDesigner {

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
    QCheckBox *m_toolsCheckBox;
    Utils::PathChooser *m_pathChooserExamples;
    Utils::PathChooser *m_pathChooserBundles;
};

class QMLDESIGNERBASE_EXPORT StudioConfigSettingsPage : public Core::IOptionsPage
{
public:
    StudioConfigSettingsPage();
};

class QMLDESIGNERBASE_EXPORT QmlDesignerBasePlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "QmlDesignerBase.json")

public:
    QmlDesignerBasePlugin();
    ~QmlDesignerBasePlugin();

    static QmlDesignerBasePlugin *instance();

    static Utils::FilePath defaultExamplesPath();
    static Utils::FilePath defaultBundlesPath();
    static QString examplesPathSetting();
    static QString bundlesPathSetting();

    static class DesignerSettings &settings();

signals:
    void examplesDownloadPathChanged(const QString &path);
    void bundlesDownloadPathChanged(const QString &path);

private:
    bool initialize(const QStringList &arguments, QString *errorMessage) override;

private:
    class Data;
    std::unique_ptr<Data> d;
};

} // namespace QmlDesigner
