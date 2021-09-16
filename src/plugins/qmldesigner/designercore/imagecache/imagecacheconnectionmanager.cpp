/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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
****************************************************************************/

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

    if (command.userType() == capturedDataCommandType) {
        m_captureCallback(command.value<CapturedDataCommand>().image);
        m_capturedDataArrived = true;
    }
}

} // namespace QmlDesigner
