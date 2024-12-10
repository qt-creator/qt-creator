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
// This file is a part of the QuickQanava software library.
//
// \file	qanRightResizer.h
// \author	benoit@destrat.io
// \date	2022 10 09
//-----------------------------------------------------------------------------

#pragma once

// Qt headers
#include <QtQml>
#include <QQuickItem>

namespace qan {  // ::qan

/*! \brief Add a resize handler ont the right of a target QML Item.
 *
 * \warning This component must not be parented to it's target.
 *
 * \nosubgrouping
 */
class RightResizer : public QQuickItem
{
    /*! \name RightResizer Object Management *///------------------------------
    //@{
    Q_OBJECT
    QML_ELEMENT

public:
    //! .
    explicit RightResizer(QQuickItem* parent = nullptr);
    virtual ~RightResizer() override = default;
    RightResizer(const RightResizer&) = delete;
    //@}
    //-------------------------------------------------------------------------

    /*! \name Resizer Management *///------------------------------------------
    //@{
public:
    //! Item that should be resized by this resizer (set to nullptr to disable the resizer).
    Q_PROPERTY(QQuickItem* target READ getTarget WRITE setTarget NOTIFY targetChanged FINAL)
    void        setTarget(QQuickItem* target);
    QQuickItem* getTarget() const { return _target.data(); }
signals:
    void        targetChanged();
private:
    QPointer<QQuickItem>  _target = nullptr;

public:
    //! FIXME
    Q_PROPERTY(QQuickItem* targetContent READ getTargetContent WRITE setTargetContent NOTIFY targetContentChanged FINAL)
    void        setTargetContent(QQuickItem* targetContent);
    QQuickItem* getTargetContent() const { return _targetContent.data(); }
signals:
    void        targetContentChanged();
private:
    QPointer<QQuickItem>  _targetContent = nullptr;

private slots:
    void        onTargetXChanged();
    void        onTargetYChanged();
    void        onTargetWidthChanged();
    void        onTargetHeightChanged();
    void        onUpdate();

public:
    //! Target could not be resized below \c minimumTargetSize (default to \c invalid ie, no constrain).
    Q_PROPERTY(QSizeF minimumTargetSize READ getMinimumTargetSize WRITE setMinimumTargetSize NOTIFY minimumTargetSizeChanged FINAL)
    //! \sa minimumTargetSize
    void        setMinimumTargetSize(QSizeF minimumTargetSize);
    //! \sa minimumTargetSize
    QSizeF      getMinimumTargetSize() const { return _minimumTargetSize; }
signals:
    //! \sa minimumTargetSize
    void        minimumTargetSizeChanged();
private:
    //! \sa minimumTargetSize
    QSizeF      _minimumTargetSize{};

public:
    // FIXME
    //! Set to true to prevent resizing that would modify the height/width ratio defined in \c ratio.
    Q_PROPERTY(bool preserveRatio READ getPreserveRatio WRITE setPreserveRatio NOTIFY preserveRatioChanged FINAL)
    void        setPreserveRatio(bool preserveRatio) noexcept;
    bool        getPreserveRatio() const noexcept { return _preserveRatio; }
signals:
    void        preserveRatioChanged();
private:
    bool        _preserveRatio = false;

public:
    //! Set to true to prevent resizing that would modify the height/width ratio defined in \c ratio.
    Q_PROPERTY(qreal ratio READ getRatio WRITE setRatio NOTIFY ratioChanged FINAL)
    void        setRatio(qreal ratio) noexcept;
    qreal       getRatio() const noexcept { return _ratio; }
signals:
    void        ratioChanged();
private:
    qreal       _ratio = 1.0;
    //@}
    //-------------------------------------------------------------------------

    /*! \name Resizer Management *///------------------------------------------
    //@{
signals:
    //! Emitted immediately before a resize operation start, \c targetSize is target item current size.
    void        resizeStart(QSizeF targetSize);
    //! Emitted immediately after a resize operation, \c targetSize is target item size after resize.
    void        resizeEnd(QSizeF targetSize);
protected:
    virtual void    hoverEnterEvent(QHoverEvent *event) override;
    virtual void    hoverLeaveEvent(QHoverEvent *event) override;
    virtual void    mouseMoveEvent(QMouseEvent* event) override;
    virtual void    mousePressEvent(QMouseEvent* event) override;
    virtual void    mouseReleaseEvent(QMouseEvent* event) override;
private:
    //! Initial global mouse position at the beginning of a resizing handler drag.
    QPointF     _dragInitialPos{0., 0.};
    //! Target item size at the beginning of a resizing handler drag.
    QSizeF      _targetInitialSize{0., 0.};
    //@}
    //-------------------------------------------------------------------------
};

}  // ::qan

QML_DECLARE_TYPE(qan::RightResizer);
