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

Q_DECLARE_METATYPE(Gerrit::Internal::GerritServer);

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

    m_resetRemoteButton = new QToolButton(this);
    m_resetRemoteButton->setToolTip(tr("Refresh Remote Servers"));

    horizontalLayout->addWidget(m_resetRemoteButton);

    connect(m_remoteComboBox, &QComboBox::currentTextChanged,
            this, &GerritRemoteChooser::handleRemoteChanged);
    m_resetRemoteButton->setIcon(Utils::Icons::RESET_TOOLBAR.icon());
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

bool GerritRemoteChooser::updateRemotes(bool forceReload)
{
    QTC_ASSERT(!m_repository.isEmpty(), return false);
    m_remoteComboBox->clear();
    m_updatingRemotes = true;
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
    addRemote(m_parameters->server, tr("Fallback"));
    m_updatingRemotes = false;
    handleRemoteChanged();
    return true;
}

void GerritRemoteChooser::addRemote(const GerritServer &server, const QString &name)
{
    for (int i = 0, total = m_remoteComboBox->count(); i < total; ++i) {
        const GerritServer s = m_remoteComboBox->itemData(i).value<GerritServer>();
        if (s == server)
            return;
    }
    m_remoteComboBox->addItem(server.host + QString(" (%1)").arg(name), QVariant::fromValue(server));
    if (name == "gerrit")
        m_remoteComboBox->setCurrentIndex(m_remoteComboBox->count() - 1);
}

GerritServer GerritRemoteChooser::currentServer() const
{
    return m_remoteComboBox->currentData().value<GerritServer>();
}

void GerritRemoteChooser::handleRemoteChanged()
{
    if (m_updatingRemotes || m_remoteComboBox->count() == 0)
        return;
    emit remoteChanged();
}

} // namespace Internal
} // namespace Gerrit
