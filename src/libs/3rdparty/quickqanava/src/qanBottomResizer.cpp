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
// \file	qanBottomResizer.cpp
// \author	benoit@destrat.io
// \date	2022 10 09
//-----------------------------------------------------------------------------

// Qt headers
#include <QCursor>
#include <QMouseEvent>

// QuickQanava headers
#include "./qanBottomResizer.h"

namespace qan {  // ::qan

/* BottomResizer Object Management *///----------------------------------------
BottomResizer::BottomResizer(QQuickItem* parent) :
    QQuickItem{parent}
{
    setAcceptedMouseButtons(Qt::LeftButton | Qt::RightButton);
    setAcceptHoverEvents(true);
    setZ(100000000.);
}
//-----------------------------------------------------------------------------

/* Resizer Management *///-----------------------------------------------------
void    BottomResizer::setTarget(QQuickItem* target)
{
    if (_target != target) {
        if (_target)
            disconnect(_target.data(),  nullptr,
                       this,            nullptr);  // Disconnect old target width/height monitoring
        _target = target;
        emit targetChanged();
    }
    if (_target) {
        connect(_target,    &QQuickItem::xChanged,
                this,       &BottomResizer::onTargetXChanged);
        connect(_target,    &QQuickItem::yChanged,
                this,       &BottomResizer::onTargetYChanged);

        connect(_target,    &QQuickItem::widthChanged,
                this,       &BottomResizer::onTargetWidthChanged);
        connect(_target,    &QQuickItem::heightChanged,
                this,       &BottomResizer::onTargetHeightChanged);
        connect(_target,    &QQuickItem::parentChanged, // When a selected group is dragged in another
                this,       &BottomResizer::onUpdate);   // parent group, resize area has to be updated

        connect(_target,    &QQuickItem::visibleChanged,
                this,       &BottomResizer::onUpdate);
        connect(_target,    &QQuickItem::zChanged,
                this,       &BottomResizer::onUpdate);
        connect(_target,    &QObject::destroyed,
                this,       [this]() {
            setTarget(nullptr);
            this->setVisible(false);
        });

        onUpdate();
    }
    setVisible(_target != nullptr &&
               _target->isVisible());
}

void    BottomResizer::setTargetContent(QQuickItem* targetContent)
{
    if (_targetContent != targetContent) {
        _targetContent = targetContent;
        emit targetChanged();
    }
}

void    BottomResizer::onTargetXChanged()
{
    if (_target && parentItem() != nullptr) {
        const auto sp = _target->mapToItem(parentItem(), QPointF{0, 0});
        setX(sp.x());
    }
}

void    BottomResizer::onTargetYChanged()
{
    if (_target && parentItem() != nullptr) {
        const auto sp = _target->mapToItem(parentItem(), QPointF{0, 0});
        setY(sp.y() + _target->height() - 3.5);
    }
}

void    BottomResizer::onTargetWidthChanged()
{
    if (_target)
        setWidth(_target->width() - 4);   // Account for a potential bottom right resizer
}

void    BottomResizer::onTargetHeightChanged()
{
    if (_target && parentItem() != nullptr) {
        const auto sp = _target->mapToItem(parentItem(), QPointF{0, 0});
        setY(sp.y() + _target->height() - 3.5);
        setHeight(7.);
    }
}

void    BottomResizer::onUpdate()
{
    onTargetXChanged();
    onTargetYChanged();
    onTargetWidthChanged();
    onTargetHeightChanged();
    setVisible(_target != nullptr &&
               _target->isVisible());
}

void    BottomResizer::setMinimumTargetSize(QSizeF minimumTargetSize)
{
    if (minimumTargetSize != _minimumTargetSize) {
        _minimumTargetSize = minimumTargetSize;
        emit minimumTargetSizeChanged();
    }
}

void    BottomResizer::setPreserveRatio(bool preserveRatio) noexcept
{
    if (preserveRatio != _preserveRatio) {
        _preserveRatio = preserveRatio;
        emit preserveRatioChanged();
    }
}

void    BottomResizer::setRatio(qreal ratio) noexcept
{
    if (!qFuzzyCompare(2.0 + _ratio, 2.0 + ratio)) { // Using 2.0 because -1.0 is a valid input (disable ratio...)
        _ratio = ratio;
        emit ratioChanged();
    }
}
//-----------------------------------------------------------------------------

/* Resizer Management *///-----------------------------------------------------
void    BottomResizer::hoverEnterEvent(QHoverEvent *event)
{
    if (isVisible()) {
        setCursor(Qt::SplitVCursor);
        event->setAccepted(true);
    }
}
void    BottomResizer::hoverLeaveEvent(QHoverEvent *event)
{
    setCursor(Qt::ArrowCursor);
    event->setAccepted(true);
}
void    BottomResizer::mouseMoveEvent(QMouseEvent* event)
{
    const auto mePos = event->scenePosition();
    if (event->buttons() |  Qt::LeftButton &&
            !_dragInitialPos.isNull() &&
            !_targetInitialSize.isEmpty()) {
        const QPointF startLocalPos = parentItem() != nullptr ? parentItem()->mapFromScene(_dragInitialPos) :
                                                                QPointF{.0, 0.};
        const QPointF curLocalPos = parentItem() != nullptr ? parentItem()->mapFromScene(mePos) :
                                                              QPointF{0., 0.};
        const QPointF delta{curLocalPos - startLocalPos};
        if (_target != nullptr) {
            const qreal targetHeight = _targetInitialSize.height() + delta.y();

            auto childrenRect = _targetContent ? _targetContent->childrenRect() : QRectF{};
            if (childrenRect.size().isEmpty())  // Note 20231208: Fix a nasty bug (Qt 5.15.13 ?) where size() is empty when there
                childrenRect = QRectF{};   // is no longer any childs but rect position is left with invalid value.
            const auto targetContentMinHeight = _targetContent ? childrenRect.y() + childrenRect.height() : 0;
            const auto minimumTargetHeight = qMax(_minimumTargetSize.height(), targetContentMinHeight);
            const auto targetContentMinWidth = _targetContent ? childrenRect.x() + childrenRect.width() : 0;
            const auto minimumTargetWidth = qMax(_minimumTargetSize.width(), targetContentMinWidth);

            if (targetHeight > minimumTargetHeight) {   // Do not resize below minimumTargetSize
                _target->setHeight(targetHeight);
                if (_preserveRatio) {
                    const qreal targetWidth = targetHeight / getRatio();
                    if (targetWidth > minimumTargetWidth)
                        _target->setWidth(targetWidth);
                }
            }
            event->setAccepted(true);
        }
    }
}
void    BottomResizer::mousePressEvent(QMouseEvent* event)
{
    if (!isVisible())
        return;
    if (_target) {
        _dragInitialPos = event->windowPos();
        _targetInitialSize = {_target->width(), _target->height()};
        emit resizeStart(_target ? QSizeF{_target->width(), _target->height()} :  // Use of target ok.
                                  QSizeF{});
        event->setAccepted(true);
    }
}
void    BottomResizer::mouseReleaseEvent(QMouseEvent* event)
{
    Q_UNUSED(event)
    _dragInitialPos = {0., 0.};       // Invalid all cached coordinates when button is released
    _targetInitialSize = {0., 0.};
    if (_target)
        emit resizeEnd(_target ? QSizeF{_target->width(), _target->height()} :  // Use of _target ok.
                                 QSizeF{});
}
//-------------------------------------------------------------------------

} // ::qan
