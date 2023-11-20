// Copyright (C) 2016 Denis Mingulov.
// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "imageviewer.h"

#include <extensionsystem/iplugin.h>

namespace ImageViewer::Internal {

class ImageViewerPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "ImageViewer.json")

public:
    void initialize() final
    {
        setupImageViewer(this);
    }
};

} // ImageViewer::Internal

#include "imageviewerplugin.moc"
