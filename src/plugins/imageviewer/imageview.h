/**************************************************************************
**
** Copyright (C) 2015 Denis Mingulov.
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator
**
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd and its Subsidiary(-ies) nor
**     the names of its contributors may be used to endorse or promote
**     products derived from this software without specific prior written
**     permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
**************************************************************************/


#ifndef IMAGEVIEW_H
#define IMAGEVIEW_H

#include <QGraphicsView>

namespace ImageViewer {
namespace Internal {

class ImageViewerFile;

class ImageView : public QGraphicsView
{
    Q_OBJECT

public:
    ImageView(ImageViewerFile *file);
    ~ImageView();

    void reset();
    void createScene();

signals:
    void scaleFactorChanged(qreal factor);
    void imageSizeChanged(const QSize &size);

public slots:
    void setViewBackground(bool enable);
    void setViewOutline(bool enable);
    void zoomIn();
    void zoomOut();
    void resetToOriginalSize();
    void fitToScreen();

private slots:
    void emitScaleFactor();

protected:
    void drawBackground(QPainter *p, const QRectF &rect);
    void hideEvent(QHideEvent *event);
    void showEvent(QShowEvent *event);
    void wheelEvent(QWheelEvent *event);

private:
    void doScale(qreal factor);

    ImageViewerFile *m_file;
    QGraphicsItem *m_imageItem = 0;
    QGraphicsRectItem *m_backgroundItem = 0;
    QGraphicsRectItem *m_outlineItem = 0;
    bool m_showBackground = false;
    bool m_showOutline = true;
};

} // namespace Internal
} // namespace ImageViewer

#endif // IMAGEVIEW_H
