// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "import3dconnectionmanager.h"

#include <imagecontainer.h>
#include <puppettocreatorcommand.h>

#include <QImage>

namespace QmlDesigner {

Import3dConnectionManager::Import3dConnectionManager()
{
    connections().clear(); // Remove default interactive puppets
    connections().emplace_back("Import 3D", "import3dmode");
}

void Import3dConnectionManager::setPreviewIconCallback(IconCallback callback)
{
    m_previewIconCallback = std::move(callback);
}

void Import3dConnectionManager::setPreviewImageCallback(ImageCallback callback)
{
    m_previewImageCallback = std::move(callback);
}

void Import3dConnectionManager::dispatchCommand(const QVariant &command,
                                                ConnectionManagerInterface::Connection &connection)
{
    static const int commandType = QMetaType::fromName("PuppetToCreatorCommand").id();;

    if (command.typeId() == commandType) {
        auto cmd = command.value<PuppetToCreatorCommand>();
        switch (cmd.type()) {
        case PuppetToCreatorCommand::Import3DPreviewIcon: {
            const QVariantList data = cmd.data().toList();
            const QString assetName = data[0].toString();
            ImageContainer container = qvariant_cast<ImageContainer>(data[1]);
            QImage image = container.image();
            if (!image.isNull())
                m_previewIconCallback(assetName, image);
            break;
        }
        case PuppetToCreatorCommand::Import3DPreviewImage: {
            ImageContainer container = qvariant_cast<ImageContainer>(cmd.data());
            QImage image = container.image();
            if (!image.isNull())
                m_previewImageCallback(image);
            break;
        }
        default:
            break;
        }
    } else {
        InteractiveConnectionManager::dispatchCommand(command, connection);
    }
}

} // namespace QmlDesigner
