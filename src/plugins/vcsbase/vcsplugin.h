// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <extensionsystem/iplugin.h>

#include <QFuture>

QT_BEGIN_NAMESPACE
class QStandardItemModel;
QT_END_NAMESPACE

namespace Utils { class FutureSynchronizer; }

namespace VcsBase {

class VcsBaseSubmitEditor;

namespace Internal {

class CommonVcsSettings;

class VcsPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "VcsBase.json")

public:
    VcsPlugin();
    ~VcsPlugin() override;

    bool initialize(const QStringList &arguments, QString *errorMessage) override;

    static VcsPlugin *instance();

    CommonVcsSettings &settings() const;

    static Utils::FutureSynchronizer *futureSynchronizer();

    // Model of user nick names used for the submit
    // editor. Stored centrally here to achieve delayed
    // initialization and updating on settings change.
    QStandardItemModel *nickNameModel();

signals:
    void settingsChanged();
    void submitEditorAboutToClose(VcsBase::VcsBaseSubmitEditor *e, bool *result);

private:
    void slotSettingsChanged();
    void populateNickNameModel();

    class VcsPluginPrivate *d = nullptr;
};

} // namespace Internal
} // namespace VcsBase
