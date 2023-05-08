// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/filepath.h>

#include <QObject>
#include <QPointer>
#include <QSharedPointer>
#include <QPair>

namespace Core {
class ActionContainer;
class Command;
class CommandLocator;
}

namespace VcsBase { class VcsBasePluginState; }

namespace Gerrit {
namespace Internal {

class GerritChange;
class GerritDialog;
class GerritParameters;
class GerritServer;
class GerritOptionsPage;

class GerritPlugin : public QObject
{
    Q_OBJECT

public:
    GerritPlugin();
    ~GerritPlugin() override;

    void addToMenu(Core::ActionContainer *ac);

    static Utils::FilePath gitBinDirectory();
    static QString branch(const Utils::FilePath &repository);
    void addToLocator(Core::CommandLocator *locator);
    void push(const Utils::FilePath &topLevel);

    void updateActions(const VcsBase::VcsBasePluginState &state);

signals:
    void fetchStarted(const QSharedPointer<Gerrit::Internal::GerritChange> &change);
    void fetchFinished();

private:
    void openView();
    void push();

    Utils::FilePath findLocalRepository(const QString &project, const QString &branch) const;
    void fetch(const QSharedPointer<GerritChange> &change, int mode);

    QSharedPointer<GerritParameters> m_parameters;
    QSharedPointer<GerritServer> m_server;
    QPointer<GerritDialog> m_dialog;
    Core::Command *m_gerritCommand = nullptr;
    Core::Command *m_pushToGerritCommand = nullptr;
    QString m_reviewers;
    GerritOptionsPage *m_gerritOptionsPage = nullptr;
};

} // namespace Internal
} // namespace Gerrit
