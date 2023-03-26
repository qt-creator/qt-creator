// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <designercore/instances/connectionmanager.h>

namespace QmlDesigner {

class CapturedDataCommand;

class ImageCacheConnectionManager : public ConnectionManager
{
public:
    using Callback = std::function<void(const QImage &)>;

    ImageCacheConnectionManager();

    void setCallback(Callback captureCallback);

    bool waitForCapturedData();

protected:
    void dispatchCommand(const QVariant &command, Connection &connection) override;

private:
    Callback m_captureCallback;
    bool m_capturedDataArrived = false;
};

} // namespace QmlDesigner
