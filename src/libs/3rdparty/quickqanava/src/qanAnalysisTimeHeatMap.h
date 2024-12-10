/*
 Copyright (c) 2008-2023, Benoit AUTHEMAN All rights reserved.

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
// \date	2017 06 04 (refactored / opensourced 2022 08 10)
//-----------------------------------------------------------------------------

#pragma once

// Qt headers
#include <QQuickPaintedItem>
#include <QImage>

// QuickQanava headers
#include "./qanNavigablePreview.h"

namespace qan { // ::qan

/*! \brief Draw an heat map from a qan::NavigablePreview, highlight parts of the image where the analyst has spent the more viewing time.
 *
 * The longer an analyst is viewing part of navigable, the hottest the heatmap will be at that location. Only
 * zoom > 100% will be taken into account for heat generation, viewing a navigable at lower zoom will not affect
 * heatmap, a navigable part is not considered analyzed until it has been viewed at full resolution.
 *
 * Call resetHeatMap() to remove all heat informations and clear the maximum analysis duration.
 *
 * \note AnalysisTimeHeatMap can be resized on the fly, resolution of the heatmap is mapped to item size.
 *
 * Internals: Analysis time heat map catch \c source qan::NavigablePreview::visibleWindowChanged() signal and
 * draw more heat on the image part covered by actual visible window.
 *
 * \nosubgrouping
 */
class AnalysisTimeHeatMap : public QQuickPaintedItem
{
    /*! \name AnalysisTimeHeatMap Object Management *///-----------------------
    //@{
    Q_OBJECT
    QML_ELEMENT
public:
    explicit AnalysisTimeHeatMap(QQuickItem* parent = nullptr);
    virtual ~AnalysisTimeHeatMap() = default;
    AnalysisTimeHeatMap(const AnalysisTimeHeatMap&) = delete;

public:
    virtual void    paint(QPainter* painter) override;

public:
    Q_PROPERTY(QImage image READ getImage WRITE setImage NOTIFY imageChanged)
    inline const QImage&    getImage() const noexcept { return _image; }
    inline QImage&          getImage() noexcept { return _image; }
    void                    setImage(QImage image) noexcept;
private:
    QImage                  _image;
signals:
    void                    imageChanged();
    //@}
    //-------------------------------------------------------------------------

    /*! \name Heatmap Generation Management *///-------------------------------
    //@{
public:
    /*! \brief Source qan::NavigablePreview, source qan::NavigablePreview::visibleWindowChanged() signal is caught.
     *
     * \warning Can be nullptr.
     */
    Q_PROPERTY(qan::NavigablePreview* source READ getSource WRITE setSource NOTIFY sourceChanged FINAL)
    //! \copydoc source
    inline qan::NavigablePreview*   getSource() const noexcept { return _source.data(); }
    //! \copydoc source
    void                            setSource(qan::NavigablePreview* source) noexcept;
private:
    //! \copydoc source
    QPointer<qan::NavigablePreview> _source;
signals:
    //! \copydoc source
    void                            sourceChanged();

public:
    //! Color used to highlight analyzed areas (alpha component is ignored, default to green).
    Q_PROPERTY( QColor color READ getColor WRITE setColor NOTIFY colorChanged FINAL )
    //! \copydoc color
    inline QColor   getColor() const noexcept { return _color; }
    //! \warning Changing color dynamically is quite costly (0(width*height)).
    void            setColor(QColor color) noexcept;
private:
    //! \copydoc color
    QColor          _color{0,255,0};
signals:
    //! \copydoc color
    void            colorChanged();

protected slots:
    //! Update heatmap when \c source qan::NavigablePreview::visibleWindowChanged() signal is emitted.
    void        onVisibleWindowChanged(QRectF visibleWindowRect, qreal navigableZoom);

    //! Clear all heatmap content (also clear maximum analysis duration information).
    Q_INVOKABLE void    clearHeatMap() noexcept;

private:
    //! Maximum analysis time on a heatmap pixel in ms.
    qint64      _maximumAnalysisDuration = -1;

protected:
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    //! Resize the internal heatmap image.
    virtual void    geometryChanged(const QRectF& newGeometry, const QRectF& oldGeometry) override;
#else
    virtual void    geometryChange(const QRectF& newGeometry, const QRectF& oldGeometry) override;
#endif
    //@}
    //-------------------------------------------------------------------------
};

} // ::qan

QML_DECLARE_TYPE(qan::AnalysisTimeHeatMap)
