// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "bakelightsconnectionmanager.h"

#include "qmldesignertr.h"

#include <puppettocreatorcommand.h>

namespace QmlDesigner {

BakeLightsConnectionManager::BakeLightsConnectionManager()
{
    connections().emplace_back("Bake lights", "bakelightsmode");
}

void BakeLightsConnectionManager::setProgressCallback(Callback callback)
{
    m_progressCallback = std::move(callback);
}

void BakeLightsConnectionManager::setFinishedCallback(Callback callback)
{
    m_finishedCallback = std::move(callback);
}

void BakeLightsConnectionManager::dispatchCommand(const QVariant &command,
                                                  ConnectionManagerInterface::Connection &)
{
    static const int commandType = QMetaType::type("PuppetToCreatorCommand");

    if (command.typeId() == commandType) {
        auto cmd = command.value<PuppetToCreatorCommand>();
        switch (cmd.type()) {
        case PuppetToCreatorCommand::BakeLightsProgress:
            m_progressCallback(cmd.data().toString());
            break;
        case PuppetToCreatorCommand::BakeLightsAborted:
            m_finishedCallback(Tr::tr("Baking aborted: %1").arg(cmd.data().toString()));
            break;
        case PuppetToCreatorCommand::BakeLightsFinished:
            m_finishedCallback(Tr::tr("Baking finished!"));
            break;
        default:
            break;
        }
    }
}

} // namespace QmlDesigner
