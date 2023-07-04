// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "imagecacheconnectionmanager.h"

#include <captureddatacommand.h>

#include <QLocalSocket>

namespace QmlDesigner {

ImageCacheConnectionManager::ImageCacheConnectionManager()
{
    connections().emplace_back("Capture icon", "captureiconmode");
}

void ImageCacheConnectionManager::setCallback(ImageCacheConnectionManager::Callback callback)
{
    m_captureCallback = std::move(callback);
}

bool ImageCacheConnectionManager::waitForCapturedData()
{
    if (connections().empty())
        return false;

    disconnect(connections().front().socket.get(), &QIODevice::readyRead, nullptr, nullptr);

    while (!m_capturedDataArrived) {
        if (!(connections().front().socket))
            return false;
        bool dataArrived = connections().front().socket->waitForReadyRead(10000);

        if (!dataArrived)
            return false;

        readDataStream(connections().front());
    }

    m_capturedDataArrived = false;

    return true;
}

void ImageCacheConnectionManager::dispatchCommand(const QVariant &command,
                                                  ConnectionManagerInterface::Connection &)
{
    static const int capturedDataCommandType = QMetaType::type("CapturedDataCommand");

    if (command.typeId() == capturedDataCommandType) {
        m_captureCallback(command.value<CapturedDataCommand>().image);
        m_capturedDataArrived = true;
    }
}

} // namespace QmlDesigner
