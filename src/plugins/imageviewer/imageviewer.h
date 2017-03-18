/****************************************************************************
**
** Copyright (C) 2016 Denis Mingulov.
** Copyright (C) 2016 The Qt Company Ltd.
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
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/idocument.h>

#include <QScopedPointer>
#include <QStringList>

QT_BEGIN_NAMESPACE
class QAbstractButton;
class QAction;
QT_END_NAMESPACE

namespace ImageViewer {
namespace Internal {
class ImageViewerFile;

class ImageViewer : public Core::IEditor
{
    Q_OBJECT

public:
    explicit ImageViewer(QWidget *parent = 0);
    ~ImageViewer() override;

    Core::IDocument *document() override;
    QWidget *toolBar() override;

    IEditor *duplicate() override;

    void exportImage();
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
    ImageViewer(const QSharedPointer<ImageViewerFile> &document, QWidget *parent = 0);
    void ctor();
    void playToggled();
    void updatePauseAction();

    struct ImageViewerPrivate *d;
};

} // namespace Internal
} // namespace ImageViewer
