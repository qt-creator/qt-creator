/****************************************************************************
**
** Copyright (C) 2017 Orgad Shaneh <orgads@gmail.com>.
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

#include "gerritserver.h"

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
    void setRepository(const QString  &repository);
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

    QString m_repository;
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
