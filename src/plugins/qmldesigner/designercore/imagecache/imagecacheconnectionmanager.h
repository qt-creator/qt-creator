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
