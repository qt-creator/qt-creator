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

#include "clangpchmanagerplugin.h"

#include "pchmanagerconnectionclient.h"
#include "pchmanagerclient.h"
#include "qtcreatorprojectupdater.h"

#include <coreplugin/icore.h>
#include <extensionsystem/pluginmanager.h>

#include <utils/hostosinfo.h>

namespace ClangPchManager {

namespace {

QString backendProcessPath()
{
    return Core::ICore::libexecPath()
            + QStringLiteral("/clangpchmanagerbackend")
            + QStringLiteral(QTC_HOST_EXE_SUFFIX);
}

} // anonymous namespace

class ClangPchManagerPluginData
{
public:
    PchManagerClient pchManagerClient;
    PchManagerConnectionClient connectionClient{&pchManagerClient};
    QtCreatorProjectUpdater<PchManagerProjectUpdater> projectUpdate{connectionClient.serverProxy(), pchManagerClient};
};

std::unique_ptr<ClangPchManagerPluginData> ClangPchManagerPlugin::d;

ClangPchManagerPlugin::ClangPchManagerPlugin() = default;
ClangPchManagerPlugin::~ClangPchManagerPlugin() = default;

bool ClangPchManagerPlugin::initialize(const QStringList & /*arguments*/, QString * /*errorMessage*/)
{
    d.reset(new ClangPchManagerPluginData);

    startBackend();

    return true;
}

void ClangPchManagerPlugin::extensionsInitialized()
{
}

ExtensionSystem::IPlugin::ShutdownFlag ClangPchManagerPlugin::aboutToShutdown()
{
    d->connectionClient.finishProcess();

    d.reset();

    return SynchronousShutdown;
}

void ClangPchManagerPlugin::startBackend()
{
    d->pchManagerClient.setConnectionClient(&d->connectionClient);

    d->connectionClient.setProcessPath(backendProcessPath());

    d->connectionClient.startProcessAndConnectToServerAsynchronously();
}


PchManagerClient &ClangPchManagerPlugin::pchManagerClient()
{
    return d->pchManagerClient;
}

} // namespace ClangRefactoring
