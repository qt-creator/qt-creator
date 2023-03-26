// Copyright (C) 2016 Denis Mingulov.
// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/editormanager/ieditorfactory.h>
#include <coreplugin/idocument.h>

namespace ImageViewer::Internal {

class ImageViewerFile;

class ImageViewer : public Core::IEditor
{
    Q_OBJECT

public:
    ImageViewer();
    ~ImageViewer() override;

    Core::IDocument *document() const override;
    QWidget *toolBar() override;

    IEditor *duplicate() override;

    void exportImage();
    void exportMultiImages();
    void copyDataUrl();
    void imageSizeUpdated(const QSize &size);
    void scaleFactorUpdate(qreal factor);

    void switchViewBackground();
    void switchViewOutline();
    void zoomIn();
    void zoomOut();
    void resetToOriginalSize();
    void fitToScreen();
    void updateToolButtons();
    void togglePlay();

private:
    ImageViewer(const QSharedPointer<ImageViewerFile> &document);
    void ctor();
    void playToggled();
    void updatePauseAction();

    struct ImageViewerPrivate *d;
};

class ImageViewerFactory final : public Core::IEditorFactory
{
public:
    ImageViewerFactory();
};

} // ImageViewer::Internal
