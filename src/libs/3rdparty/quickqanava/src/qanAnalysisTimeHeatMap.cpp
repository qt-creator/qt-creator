/*
 Copyright (c) 2008-2024, Benoit AUTHEMAN All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the author or Destrat.io nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL AUTHOR BE LIABLE FOR ANY
 DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

//-----------------------------------------------------------------------------
// \file	qanAnalysisTimeHeatMap.cpp
// \author	benoit@destrat.io
// \date	2017 06 04
//-----------------------------------------------------------------------------

// Qt headers
#include <QPainter>

// Topoi++ headers
#include "./qanAnalysisTimeHeatMap.h"

namespace qan { // ::qan

/* AnalysisTimeHeatMap Object Management *///----------------------------------
AnalysisTimeHeatMap::AnalysisTimeHeatMap(QQuickItem* parent) :
    QQuickPaintedItem{parent},
    _image{QImage{{0,0}, QImage::Format_ARGB32_Premultiplied}}
{
}

void    AnalysisTimeHeatMap::paint(QPainter* painter)
{
    if (!_image.isNull()) {
        // Algorithm:
            // 1. Compute source image ratio: image.height = imageRatio * image.width
            // 2. Compute xScale ratio such that: item.width = xScale * image.width
            // 3. Compute candidate height in item CS with xScale
                // 3.1 If it is < item.height, center image vertically
                // 3.2 Otherwise, compute yScale, center image horizontally

        QRectF drawRect{};
        QSizeF imageSize{static_cast<qreal>(_image.width()),
                         static_cast<qreal>(_image.height())};  // Copy image size to FP

        // 1.
        qreal imageRatio = imageSize.height() / imageSize.width();

        // 2.
        qreal xScale = width() / imageSize.width();

        // 3.
        const auto candidateHeight = imageSize.height() * xScale;
        if (candidateHeight > height()) {     // Avoid an <= FP test
            // 3.2
            const auto scaledWidth = height() / imageRatio;
            drawRect.setTopLeft({ (width() - scaledWidth) / 2., 0. });
            drawRect.setSize({ scaledWidth, height() });
        } else {
            // 3.1
            drawRect.setTopLeft({0., (height() - candidateHeight) / 2. });   // Center height
            drawRect.setSize({ width(), candidateHeight });
        }
        painter->drawImage(drawRect, _image);
    }
}

void    AnalysisTimeHeatMap::setImage(QImage image) noexcept
{
    _image = image;
    emit imageChanged();
    update();
}
//-----------------------------------------------------------------------------

/* Heatmap Generation Management *///------------------------------------------
void    AnalysisTimeHeatMap::setSource(qan::NavigablePreview* source) noexcept
{
    if (source != _source) {
        if (_source)  // Disconnect previous source from this heatmap generator
            disconnect(_source.data(), 0, this, 0);
        _source = source;
        if (_source)
            connect(_source.data(), &qan::NavigablePreview::visibleWindowChanged,
                    this,           &AnalysisTimeHeatMap::onVisibleWindowChanged);
        emit sourceChanged();
    }
}

void    AnalysisTimeHeatMap::setColor(QColor color) noexcept
{
    if (color != _color) {
        _color = color;
        // Update background image color (do not change alpha channel)
        QColor c{color};
        auto& heatMap = getImage();
        for (int x = 0; x < heatMap.width(); x++)
            for (int y = 0; y < heatMap.width(); x++) {
                c.setAlpha(heatMap.pixelColor(x, y).alpha());    // Preserve original alpha value
                heatMap.setPixelColor(x, y, c);
            }
        emit colorChanged();
        update();
    }
}

void    AnalysisTimeHeatMap::onVisibleWindowChanged(QRectF visibleWindowRect, qreal navigableZoom)
{
    if (!isVisible())
        return;
    if (navigableZoom > 1.0001 &&
        visibleWindowRect.isValid()) {
        auto& heatMap = getImage();
        const QSizeF heatMapSize{static_cast<qreal>(heatMap.width()),
                                 static_cast<qreal>(heatMap.height())};
        QRect scaledVisibleWindowRect{QRectF{ visibleWindowRect.x() * heatMapSize.width(),
                                              visibleWindowRect.y() * heatMapSize.height(),
                                              visibleWindowRect.width() * heatMapSize.width(),
                                              visibleWindowRect.height() * heatMapSize.height() }.toRect()};
        const auto heatMapRect = QRect{{0,0}, heatMap.size()};
        //qDebug() << "\tvisibleWindowRect=" << visibleWindowRect;
        //qDebug() << "\tscaledVisibleWindowRect=" << scaledVisibleWindowRect;
        //qDebug() << "\theatMapSize=" << heatMapSize;

        QColor c{_color};
        if (heatMapRect.intersects(scaledVisibleWindowRect)) { // Update pixels under visibleWindowRect
            const auto updateRect = heatMapRect.intersected(scaledVisibleWindowRect);
            for (int x = updateRect.left(); x < updateRect.right(); x++)
                for (int y = updateRect.top(); y < updateRect.bottom(); y++) {
                    const auto alpha = heatMap.pixelColor(x,y).alpha();
                    c.setAlpha(qMax(25, qMin(255, alpha+1)));
                    heatMap.setPixelColor(x, y, c);
                }
            update();
        }
    }
}

void    AnalysisTimeHeatMap::clearHeatMap() noexcept
{
    auto& heatMap = getImage();
    QColor c{_color}; c.setAlpha(0);
    QImage clearedHeatMap{heatMap};
    clearedHeatMap.fill(c);
    heatMap = clearedHeatMap;
    _maximumAnalysisDuration = -1;
    update();
}

void    AnalysisTimeHeatMap::geometryChange(const QRectF& newGeometry, const QRectF& oldGeometry)
{
    if (!newGeometry.size().toSize().isEmpty() &&
        newGeometry.toRect() != oldGeometry.toRect()) {
        auto& heatMap = getImage();
        if (heatMap.isNull()) {
            heatMap = QImage{newGeometry.size().toSize(), QImage::Format_ARGB32_Premultiplied};
            QColor c{_color}; c.setAlpha(0);
            heatMap.fill(c);
        } else {
            if (newGeometry.size().width() > oldGeometry.size().width() ||
                newGeometry.size().height() > oldGeometry.size().height()) {
                QImage scaled = heatMap.scaled(newGeometry.size().toSize()); // It should detach...
                setImage(scaled);
            } // Otherwise, do not reduce image size, it will be scaled at display
        }
        update();
    }
}
//-----------------------------------------------------------------------------

} // ::qan
