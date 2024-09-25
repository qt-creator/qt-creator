// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "interactiveconnectionmanager.h"

QT_FORWARD_DECLARE_CLASS(QImage)

namespace QmlDesigner {

class Import3dConnectionManager : public InteractiveConnectionManager
{
public:
    using IconCallback = std::function<void(const QString &, const QImage &)>;
    using ImageCallback = std::function<void(const QImage &)>;

    Import3dConnectionManager();

    void setPreviewIconCallback(IconCallback callback);
    void setPreviewImageCallback(ImageCallback callback);

protected:
    void dispatchCommand(const QVariant &command, Connection &connection) override;

private:
    IconCallback m_previewIconCallback;
    ImageCallback m_previewImageCallback;
};

} // namespace QmlDesigner
