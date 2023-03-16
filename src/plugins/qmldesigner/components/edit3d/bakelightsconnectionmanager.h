// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "connectionmanager.h"

namespace QmlDesigner {

class BakeLightsConnectionManager : public ConnectionManager
{
public:
    using Callback = std::function<void(const QString &)>;

    BakeLightsConnectionManager();

    void setProgressCallback(Callback callback);
    void setFinishedCallback(Callback callback);

protected:
    void dispatchCommand(const QVariant &command, Connection &connection) override;

private:
    Callback m_progressCallback;
    Callback m_finishedCallback;
};

} // namespace QmlDesigner
