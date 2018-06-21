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

#include "gerritremotechooser.h"
#include "gerritparameters.h"
#include "gerritserver.h"
#include "../gitclient.h"
#include "../gitplugin.h"

#include <utils/qtcassert.h>
#include <utils/utilsicons.h>

#include <QFileInfo>
#include <QHBoxLayout>

namespace Gerrit {
namespace Internal {

GerritRemoteChooser::GerritRemoteChooser(QWidget *parent) :
    QWidget(parent)
{
    auto horizontalLayout = new QHBoxLayout(this);
    m_remoteComboBox = new QComboBox(this);
    QSizePolicy sizePolicy1(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
    sizePolicy1.setHorizontalStretch(0);
    sizePolicy1.setVerticalStretch(0);
    sizePolicy1.setHeightForWidth(m_remoteComboBox->sizePolicy().hasHeightForWidth());
    m_remoteComboBox->setSizePolicy(sizePolicy1);
    m_remoteComboBox->setMinimumSize(QSize(40, 0));

    horizontalLayout->addWidget(m_remoteComboBox);
    horizontalLayout->setMargin(0);

    m_resetRemoteButton = new QToolButton(this);
    m_resetRemoteButton->setToolTip(tr("Refresh Remote Servers"));

    horizontalLayout->addWidget(m_resetRemoteButton);

    connect(m_remoteComboBox, &QComboBox::currentTextChanged,
            this, &GerritRemoteChooser::handleRemoteChanged);
    m_resetRemoteButton->setIcon(Utils::Icons::RESET.icon());
    connect(m_resetRemoteButton, &QToolButton::clicked,
            this, [this] { updateRemotes(true); });
}

void GerritRemoteChooser::setRepository(const QString &repository)
{
    m_repository = repository;
}

void GerritRemoteChooser::setParameters(QSharedPointer<GerritParameters> parameters)
{
    m_parameters = parameters;
}

void GerritRemoteChooser::setFallbackEnabled(bool value)
{
    m_enableFallback = value;
}

void GerritRemoteChooser::setAllowDups(bool value)
{
    m_allowDups = value;
}

bool GerritRemoteChooser::setCurrentRemote(const QString &remoteName)
{
    for (int i = 0, total = m_remoteComboBox->count(); i < total; ++i) {
        if (m_remotes[i].first == remoteName) {
            m_remoteComboBox->setCurrentIndex(i);
            return true;
        }
    }
    return false;
}

bool GerritRemoteChooser::updateRemotes(bool forceReload)
{
    QTC_ASSERT(!m_repository.isEmpty() || !m_parameters, return false);
    m_updatingRemotes = true;
    m_remoteComboBox->clear();
    m_remotes.clear();
    QString errorMessage; // Mute errors. We'll just fallback to the defaults
    QMap<QString, QString> remotesList =
            Git::Internal::GitPlugin::client()->synchronousRemotesList(m_repository, &errorMessage);
    QMapIterator<QString, QString> mapIt(remotesList);
    while (mapIt.hasNext()) {
        mapIt.next();
        GerritServer server;
        if (!server.fillFromRemote(mapIt.value(), *m_parameters, forceReload))
            continue;
        addRemote(server, mapIt.key());
    }
    if (m_enableFallback)
        addRemote(m_parameters->server, tr("Fallback"));
    m_remoteComboBox->setEnabled(m_remoteComboBox->count() > 1);
    m_updatingRemotes = false;
    handleRemoteChanged();
    return true;
}

void GerritRemoteChooser::addRemote(const GerritServer &server, const QString &name)
{
    if (!m_allowDups) {
        for (auto remote : m_remotes) {
            if (remote.second == server)
                return;
        }
    }
    m_remoteComboBox->addItem(server.host + QString(" (%1)").arg(name));
    m_remotes.push_back({ name, server });
    if (name == "gerrit")
        m_remoteComboBox->setCurrentIndex(m_remoteComboBox->count() - 1);
}

GerritServer GerritRemoteChooser::currentServer() const
{
    const int index = m_remoteComboBox->currentIndex();
    QTC_ASSERT(index >= 0 && index < int(m_remotes.size()), return GerritServer());
    return m_remotes[index].second;
}

QString GerritRemoteChooser::currentRemoteName() const
{
    const int index = m_remoteComboBox->currentIndex();
    QTC_ASSERT(index >= 0 && index < int(m_remotes.size()), return QString());
    return m_remotes[index].first;
}

bool GerritRemoteChooser::isEmpty() const
{
    return m_remotes.empty();
}

void GerritRemoteChooser::handleRemoteChanged()
{
    if (m_updatingRemotes || m_remotes.empty())
        return;
    emit remoteChanged();
}

} // namespace Internal
} // namespace Gerrit
