// Copyright (C) 2017 Orgad Shaneh <orgads@gmail.com>.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "gerritremotechooser.h"
#include "gerritparameters.h"
#include "gerritserver.h"
#include "../gitclient.h"
#include "../gittr.h"

#include <utils/filepath.h>
#include <utils/qtcassert.h>
#include <utils/utilsicons.h>

#include <QFileInfo>
#include <QHBoxLayout>

using namespace Utils;

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
    horizontalLayout->setContentsMargins(0, 0, 0, 0);

    m_resetRemoteButton = new QToolButton(this);
    m_resetRemoteButton->setToolTip(Git::Tr::tr("Refresh Remote Servers"));

    horizontalLayout->addWidget(m_resetRemoteButton);

    connect(m_remoteComboBox, &QComboBox::currentTextChanged,
            this, &GerritRemoteChooser::handleRemoteChanged);
    m_resetRemoteButton->setIcon(Utils::Icons::RESET.icon());
    connect(m_resetRemoteButton, &QToolButton::clicked,
            this, [this] { updateRemotes(true); });
}

void GerritRemoteChooser::setRepository(const FilePath &repository)
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
    const QMap<QString, QString> remotesList =
            Git::Internal::gitClient().synchronousRemotesList(m_repository, &errorMessage);
    for (auto mapIt = remotesList.cbegin(), end = remotesList.cend(); mapIt != end; ++mapIt) {
        GerritServer server;
        if (!server.fillFromRemote(mapIt.value(), *m_parameters, forceReload))
            continue;
        addRemote(server, mapIt.key());
    }
    if (m_enableFallback)
        addRemote(m_parameters->server, Git::Tr::tr("Fallback"));
    m_remoteComboBox->setEnabled(m_remoteComboBox->count() > 1);
    m_updatingRemotes = false;
    handleRemoteChanged();
    return true;
}

void GerritRemoteChooser::addRemote(const GerritServer &server, const QString &name)
{
    if (!m_allowDups) {
        for (const auto &remote : std::as_const(m_remotes)) {
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
