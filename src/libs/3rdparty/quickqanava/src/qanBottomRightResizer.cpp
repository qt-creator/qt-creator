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
// \file	qanBottomRightResizer.cpp
// \author	benoit@destrat.io
// \date	2016 07 08
//-----------------------------------------------------------------------------

// Std headers
#include <sstream>

// Qt headers
#include <QCursor>
#include <QMouseEvent>

// QuickQanava headers
#include "./qanBottomRightResizer.h"

namespace qan {  // ::qan

/* BottomRightResizer Object Management *///-----------------------------------
BottomRightResizer::BottomRightResizer(QQuickItem* parent) :
    QQuickItem{parent}
{
    setAcceptedMouseButtons(Qt::LeftButton | Qt::RightButton);
    setAcceptHoverEvents(true);
    setZ(100000000.);
}

BottomRightResizer::~BottomRightResizer()
{
    if (_handler &&
        QQmlEngine::objectOwnership(_handler.data()) == QQmlEngine::CppOwnership)
        _handler->deleteLater();
}
//-----------------------------------------------------------------------------

/* Resizer Management *///-----------------------------------------------------
void    BottomRightResizer::setHandler(QQuickItem* handler)
{
    if (handler != _handler.data()) {
        if (_handler) {     // Delete existing handler
            if (QQmlEngine::objectOwnership(_handler.data()) == QQmlEngine::CppOwnership)
                _handler.data()->deleteLater();
        }
        _handler = handler;
        if (_handler)
            _handler->installEventFilter(this);
        emit handlerChanged();
    }
    if (_target)      // Force target reconfiguration for new handler
        configureTarget(*_target);
}

QQuickItem* BottomRightResizer::getHandler() const { return (_handler ? _handler.data() : nullptr); }

void    BottomRightResizer::setTarget(QQuickItem* target)
{
    if (target != _target) {
        if (_target != nullptr)
            disconnect(_target.data(),  nullptr,
                       this,            nullptr );  // Disconnect old target width/height monitoring

        if (!_handler) {  // Eventually, create the handler component
            QQmlEngine* engine = qmlEngine(this);
            if (engine != nullptr) {
                QQmlComponent defaultHandlerComponent{engine};
                const QString handlerQml{ QStringLiteral("import QtQuick 2.7\n  Rectangle {") +
                            QStringLiteral("width:")    + QString::number(_handlerSize.width()) + QStringLiteral(";") +
                            QStringLiteral("height:")   + QString::number(_handlerSize.height()) + QStringLiteral(";") +
                            QStringLiteral("border.width:4;radius:3;") +
                            QStringLiteral("border.color:\"") + _handlerColor.name() + QStringLiteral("\";") +
                            QStringLiteral("color:Qt.lighter(border.color); }") };
                defaultHandlerComponent.setData(handlerQml.toUtf8(), QUrl{});
                if (defaultHandlerComponent.isReady()) {
                    _handler = qobject_cast<QQuickItem*>(defaultHandlerComponent.create());
                    if (_handler) {
                        engine->setObjectOwnership(_handler.data(), QQmlEngine::CppOwnership);
                        _handler->setParentItem(this);
                        //_handler->installEventFilter(this);
                    } else {
                        qWarning() << "qan::BottomRightResizer::setTarget(): Error: Can't create resize handler QML component:";
                        qWarning() << "QML Component status=" << defaultHandlerComponent.status();
                    }
                }
            }
            QObject* handlerBorder = _handler->property("border").value<QObject*>();
            if (handlerBorder != nullptr)
                handlerBorder->setProperty("width", _handlerWidth);
            if (_handler)
                _handler->setProperty("radius", _handlerRadius);
            if (_handler)
                _handler->setSize(_handlerSize);
        }

        _target = target;
        emit targetChanged();

        // Configure handler on given target
        if (_handler)
            configureHandler(*_handler);
        if (_target)
            configureTarget(*_target);
    }
    setVisible(_target != nullptr);
}

void    BottomRightResizer::setTargetContent(QQuickItem* targetContent)
{
    if (_targetContent != targetContent) {
        _targetContent = targetContent;
        emit targetChanged();
    }
}

void    BottomRightResizer::configureHandler(QQuickItem& handler) noexcept
{
    handler.setOpacity(_autoHideHandler ? 0. : 1.);
    handler.setSize(_handlerSize);
    handler.setZ(z() + 1.);
    QObject* handlerBorder = handler.property("border").value<QObject*>();
    if (handlerBorder != nullptr)
        handlerBorder->setProperty("color", _handlerColor);
    handler.setVisible(true);
    handler.setParentItem(this);
    handler.setAcceptedMouseButtons(Qt::LeftButton);
    handler.setAcceptHoverEvents(true);
}

void    BottomRightResizer::configureTarget(QQuickItem& target) noexcept
{
    connect(&target,    &QQuickItem::xChanged,
            this,       &BottomRightResizer::onTargetXChanged);
    connect(&target,    &QQuickItem::yChanged,
            this,       &BottomRightResizer::onTargetYChanged);

    connect(&target,    &QQuickItem::widthChanged,
            this,       &BottomRightResizer::onTargetWidthChanged);
    connect(&target,    &QQuickItem::heightChanged,
            this,       &BottomRightResizer::onTargetHeightChanged);
    connect(_target,    &QQuickItem::parentChanged,         // When a selected target is dragged in another
            this,       &BottomRightResizer::onUpdate);     // parent target, resize area has to be updated

    connect(_target,    &QQuickItem::visibleChanged,
            this,       &BottomRightResizer::onUpdate);
    connect(_target,    &QQuickItem::zChanged,
            this,       &BottomRightResizer::onUpdate);
    connect(_target,    &QObject::destroyed,
            this,       [this]() {
        setTarget(nullptr);
        this->setVisible(false);
    });

    onUpdate();
}

void    BottomRightResizer::onTargetXChanged()
{
    if (_target && parentItem() != nullptr) {
        const auto sp = _target->mapToItem(parentItem(), QPointF{0, 0});
        setX(sp.x() + _target->width() - (getHandlerWidth() / 2.));
    }
}

void    BottomRightResizer::onTargetYChanged()
{
    if (_target && parentItem() != nullptr) {
        const auto sp = _target->mapToItem(parentItem(), QPointF{0, 0});
        setY(sp.y() + _target->height() - (getHandlerWidth() / 2.));
    }
}

void    BottomRightResizer::onTargetWidthChanged()
{
    if (_target && parentItem() != nullptr) {
        const auto sp = _target->mapToItem(parentItem(), QPointF{0, 0});
        setX(sp.x() + _target->width() - (getHandlerWidth() / 2.));
    }
}

void    BottomRightResizer::onTargetHeightChanged()
{
    if (_target && parentItem() != nullptr) {
        const auto sp = _target->mapToItem(parentItem(), QPointF{0, 0});
        setY(sp.y() + _target->height() - (getHandlerWidth() / 2.));
    }
}

void    BottomRightResizer::onUpdate()
{
    setWidth(_handlerSize.width());
    setHeight(_handlerSize.height());
    onTargetXChanged();
    onTargetYChanged();
    onTargetWidthChanged();
    onTargetHeightChanged();
    setVisible(_target != nullptr &&
               _target->isVisible());
}

void    BottomRightResizer::setHandlerSize(const QSizeF& handlerSize)
{
    if (handlerSize.isEmpty())
        return;
    if (handlerSize != _handlerSize) {
        _handlerSize = handlerSize;
        if (_handler)
            _handler->setSize(handlerSize);
        onUpdate();
        emit handlerSizeChanged();
    }
}

void    BottomRightResizer::setHandlerColor(QColor handlerColor)
{
    if (!handlerColor.isValid())
        return;
    if (handlerColor == _handlerColor)    // Binding loop protection
        return;
    if (_handler) {
        QObject* handlerBorder = _handler->property("border").value<QObject*>();
        if (handlerBorder)
            handlerBorder->setProperty("color", handlerColor);
    }
    _handlerColor = handlerColor;
    emit handlerColorChanged();
}

void    BottomRightResizer::setHandlerRadius(qreal handlerRadius)
{
    if (!qFuzzyCompare(1.0 + handlerRadius, 1.0 + _handlerRadius)) {
        if (_handler)
            _handler->setProperty("radius", handlerRadius);
        _handlerRadius = handlerRadius;
        emit handlerRadiusChanged();
    }
}

void    BottomRightResizer::setHandlerWidth(qreal handlerWidth)
{
    if (!qFuzzyCompare(1.0 + handlerWidth, 1.0 + _handlerWidth)) {
        QObject* handlerBorder = _handler->property("border").value<QObject*>();
        if (handlerBorder != nullptr)
            handlerBorder->setProperty("width", handlerWidth);
        _handlerWidth = handlerWidth;
        emit handlerWidthChanged();
    }
}

void    BottomRightResizer::setMinimumTargetSize(QSizeF minimumTargetSize)
{
    if (minimumTargetSize != _minimumTargetSize) {
        _minimumTargetSize = minimumTargetSize;
        emit minimumTargetSizeChanged();
    }
}

void    BottomRightResizer::setAutoHideHandler(bool autoHideHandler)
{
    if (autoHideHandler == _autoHideHandler)    // Binding loop protection
        return;
    if (_handler &&
        autoHideHandler &&
        _handler->isVisible())    // If autoHide is set to false and the handler is visible, hide it
        _handler->setVisible(false);
    _autoHideHandler = autoHideHandler;
    emit autoHideHandlerChanged();
}

void    BottomRightResizer::setPreserveRatio(bool preserveRatio) noexcept
{
    if (preserveRatio != _preserveRatio) {
        _preserveRatio = preserveRatio;
        emit preserveRatioChanged();
    }
}

void    BottomRightResizer::setRatio(qreal ratio) noexcept
{
    if (!qFuzzyCompare(2.0 + _ratio, 2.0 + ratio)) { // Using 2.0 because -1.0 is a valid input (disable ratio...)
        _ratio = ratio;
        emit ratioChanged();
    }
}
//-----------------------------------------------------------------------------

/* Resizer Management *///-----------------------------------------------------
void    BottomRightResizer::hoverEnterEvent(QHoverEvent* event)
{
    if (isVisible()) {
        setCursor(Qt::SizeFDiagCursor);
        event->setAccepted(true);
    }
}

void    BottomRightResizer::hoverLeaveEvent(QHoverEvent* event)
{
    setCursor(Qt::ArrowCursor);
    event->setAccepted(true);
}

void    BottomRightResizer::mouseMoveEvent(QMouseEvent* event)
{
    const auto mePos = event->scenePosition();
    if (event->buttons() |  Qt::LeftButton &&
            !_dragInitialPos.isNull() &&
            !_targetInitialSize.isEmpty()) {

        // Inspired by void QQuickMouseArea::mouseMoveEvent(QMouseEvent *event)
        // https://code.woboq.org/qt5/qtdeclarative/src/quick/items/qquickmousearea.cpp.html#47curLocalPos
        // Coordinate mapping in qt quick is even more a nightmare than with graphics view...
        // BTW, this code is probably buggy for deep quick item hierarchy.
        const QPointF startLocalPos = parentItem() != nullptr ? parentItem()->mapFromScene(_dragInitialPos) :
                                                                QPointF{.0, 0.};
        const QPointF curLocalPos = parentItem() != nullptr ? parentItem()->mapFromScene(mePos) :
                                                              QPointF{0., 0.};
        const QPointF delta{curLocalPos - startLocalPos};

        if (_target) {
            auto childrenRect = _targetContent ? _targetContent->childrenRect() : QRectF{};
            if (childrenRect.size().isEmpty())  // Note 20231208: Fix a nasty bug (Qt 5.15.13 ?) where size() is empty when there
                childrenRect = QRectF{};   // is no longer any childs but rect position is left with invalid value.
            const auto targetContentMinHeight = _targetContent ? childrenRect.y() + childrenRect.height() : 0;
            const auto minimumTargetHeight = qMax(_minimumTargetSize.height(), targetContentMinHeight);
            const auto targetContentMinWidth = _targetContent ? childrenRect.x() + childrenRect.width() : 0;
            const auto minimumTargetWidth = qMax(_minimumTargetSize.width(), targetContentMinWidth);

            const qreal targetWidth = _targetInitialSize.width() + delta.x();

            if (targetWidth > minimumTargetWidth)       // Do not resize below minimumSize
                _target->setWidth(targetWidth);
            if (_preserveRatio) {
                const qreal finalTargetWidth = targetWidth > minimumTargetWidth ? targetWidth :
                                                                                  minimumTargetWidth;
                const qreal targetHeight = finalTargetWidth * getRatio();
                if (targetHeight > minimumTargetHeight)
                    _target->setHeight(targetHeight);
            } else {
                const qreal targetHeight = _targetInitialSize.height() + delta.y();
                if (targetHeight > minimumTargetHeight)
                    _target->setHeight(targetHeight);
            }
            event->setAccepted(true);
        }
    }
}

void    BottomRightResizer::mousePressEvent(QMouseEvent* event)
{
    if (!isVisible())
        return;
    const auto mePos = event->scenePosition();
    if (_target) {
        _dragInitialPos = mePos;
        _targetInitialSize = QSizeF{_target->width(), _target->height()};
        emit resizeStart(_target ? QSizeF{_target->width(), _target->height()} :
                                   QSizeF{});
        event->setAccepted(true);
    }
}

void    BottomRightResizer::mouseReleaseEvent(QMouseEvent* event)
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
