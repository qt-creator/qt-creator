// Copyright (C) 2017 Orgad Shaneh <orgads@gmail.com>.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "gerritserver.h"

#include <utils/filepath.h>

#include <QComboBox>
#include <QSharedPointer>
#include <QToolButton>
#include <QWidget>

#include <vector>

namespace Gerrit {
namespace Internal {

class GerritParameters;

class GerritRemoteChooser : public QWidget
{
    Q_OBJECT

public:
    GerritRemoteChooser(QWidget *parent = nullptr);
    void setRepository(const Utils::FilePath &repository);
    void setParameters(QSharedPointer<GerritParameters> parameters);
    void setFallbackEnabled(bool value);
    void setAllowDups(bool value);
    bool setCurrentRemote(const QString &remoteName);

    bool updateRemotes(bool forceReload);
    GerritServer currentServer() const;
    QString currentRemoteName() const;
    bool isEmpty() const;

signals:
    void remoteChanged();

private:
    void addRemote(const GerritServer &server, const QString &name);
    void handleRemoteChanged();

    Utils::FilePath m_repository;
    QSharedPointer<GerritParameters> m_parameters;
    QComboBox *m_remoteComboBox = nullptr;
    QToolButton *m_resetRemoteButton = nullptr;
    bool m_updatingRemotes = false;
    bool m_enableFallback = false;
    bool m_allowDups = false;
    using NameAndServer = std::pair<QString, GerritServer>;
    std::vector<NameAndServer> m_remotes;
};

} // namespace Internal
} // namespace Gerrit
