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
// \file	qanStyle.cpp
// \author	benoit@destrat.io
// \date	2015 06 05
//-----------------------------------------------------------------------------

// Qt headers
#include <QFont>

// QuickQanava headers
#include "./qanStyle.h"

namespace qan { // ::qan

/* Style Object Management *///------------------------------------------------
Style::Style(QObject* parent) :
    QObject{parent} { /* Nil */ }

Style::Style(QString name, QObject* parent) :
    QObject{parent},
    _name{name} { /* Nil */ }
//-----------------------------------------------------------------------------

/* Node Style Object Management *///-------------------------------------------
NodeStyle::NodeStyle(QString name, QObject* parent) : qan::Style(name, parent) { /* Nil */ }
NodeStyle::NodeStyle(QObject* parent) : qan::Style(parent) { /* Nil */ }
//-----------------------------------------------------------------------------

/* Node Style Properties *///--------------------------------------------------
bool    NodeStyle::setBackRadius(qreal backRadius) noexcept
{
    // PRECONDITIONS:
        // backRadius must be >= 0.
    if (backRadius < -0.00001) {
        qWarning() << "qan::NodeStyle::setBackRadius(): Node background radius can't be < 0.";
        return false;
    }
    if (!qFuzzyCompare(1. + _backRadius, 1. + backRadius)) {
        _backRadius = backRadius;
        emit backRadiusChanged();
        return true;
    }
    return false;
}

bool    NodeStyle::setBackOpacity(qreal backOpacity) noexcept
{
    if (!qFuzzyCompare(1. + _backOpacity, 1. + backOpacity)) {
        _backOpacity = backOpacity;
        emit backOpacityChanged();
        return true;
    }
    return false;
}

bool    NodeStyle::setFillType(NodeStyle::FillType fillType) noexcept
{
    if (_fillType != fillType) {
        _fillType = fillType;
        emit fillTypeChanged();
        return true;
    }
    return false;
}

bool    NodeStyle::setBackColor(const QColor& backColor) noexcept
{
    if (_backColor != backColor) {
        _backColor = backColor;
        emit backColorChanged();
        return true;
    }
    return false;
}

bool    NodeStyle::setBaseColor(const QColor& baseColor) noexcept
{
    if (_baseColor != baseColor) {
        _baseColor = baseColor;
        emit baseColorChanged();
        return true;
    }
    return false;
}

bool    NodeStyle::setBorderColor(const QColor& borderColor) noexcept
{
    if (_borderColor != borderColor) {
        _borderColor = borderColor;
        emit borderColorChanged();
        return true;
    }
    return false;
}

bool    NodeStyle::setBorderWidth(qreal borderWidth) noexcept
{
    if (!qFuzzyCompare(1. + _borderWidth, 1. + borderWidth)) {
        _borderWidth = borderWidth;
        emit borderWidthChanged();
        return true;
    }
    return false;
}

bool    NodeStyle::setEffectType(NodeStyle::EffectType effectType) noexcept
{
    if (_effectType != effectType) {
        setEffectEnabled(effectType == EffectType::EffectNone ? false : true);
        _effectType = effectType;
        emit effectTypeChanged();
        return true;
    }
    return false;
}

bool    NodeStyle::setEffectEnabled(bool effectEnabled) noexcept
{
    if (_effectEnabled != effectEnabled) {
        _effectEnabled = effectEnabled;
        emit effectEnabledChanged();
        return true;
    }
    return false;
}

bool    NodeStyle::setEffectColor(QColor effectColor) noexcept
{
    if (_effectColor != effectColor) {
        _effectColor = effectColor;
        emit effectColorChanged();
        return true;
    }
    return false;
}

bool    NodeStyle::setEffectRadius(qreal effectRadius) noexcept
{
    if (effectRadius < 0.0)
        return false;
    if (!qFuzzyCompare(1. + _effectRadius, 1. + effectRadius)) {
        _effectRadius = effectRadius;
        emit effectRadiusChanged();
        return true;
    }
    return false;
}

bool    NodeStyle::setEffectOffset(qreal effectOffset) noexcept
{
    if ( !qFuzzyCompare( 1. + _effectOffset, 1. + effectOffset ) ) {
        _effectOffset = effectOffset;
        emit effectOffsetChanged();
        return true;
    }
    return false;
}

bool    NodeStyle::setFontPointSize(int fontPointSize) noexcept
{
    if ( _fontPointSize != fontPointSize ) {
        _fontPointSize = fontPointSize;
        emit fontPointSizeChanged();
        return true;
    }
    return false;
}

bool    NodeStyle::setFontBold(bool fontBold) noexcept
{
    if (_fontBold != fontBold) {
        _fontBold = fontBold;
        emit fontBoldChanged();
        return true;
    }
    return false;
}

bool    NodeStyle::setLabelColor(QColor labelColor) noexcept
{
    if (_labelColor != labelColor) {
        _labelColor = labelColor;
        emit labelColorChanged();
        return true;
    }
    return false;
}
//-----------------------------------------------------------------------------


/* Edge Style Object Management *///-------------------------------------------
EdgeStyle::EdgeStyle(QString name, QObject* parent) : qan::Style(name, parent) { /* Nil */ }

EdgeStyle::EdgeStyle(QObject* parent) : qan::Style(parent) { /* Nil */ }

bool    EdgeStyle::setLineType(LineType lineType) noexcept
{
    if (lineType != _lineType) {
        _lineType = lineType;
        emit lineTypeChanged();
        emit styleModified();
        return true;
    }
    return false;
}

bool    EdgeStyle::setLineColor(const QColor& lineColor) noexcept
{
    if (lineColor != _lineColor) {
        _lineColor = lineColor;
        emit lineColorChanged();
        emit styleModified();
        return true;
    }
    return false;
}

bool    EdgeStyle::setLineWidth(qreal lineWidth) noexcept
{
    if (!qFuzzyCompare(1.0 + lineWidth, 1.0 + _lineWidth)) {
        _lineWidth = lineWidth;
        emit lineWidthChanged();
        emit styleModified();
        return true;
    }
    return false;
}

bool    EdgeStyle::setArrowSize(qreal arrowSize) noexcept
{
    if (!qFuzzyCompare(1. + arrowSize, 1. + _arrowSize)) {
        _arrowSize = arrowSize;
        emit arrowSizeChanged();
        emit styleModified();
        return true;
    }
    return false;
}

auto    EdgeStyle::setSrcShape(ArrowShape srcShape) noexcept -> bool
{
    if (_srcShape != srcShape) {
        _srcShape = srcShape;
        emit srcShapeChanged();
        emit styleModified();
        return true;
    }
    return false;
}

auto    EdgeStyle::setDstShape(ArrowShape dstShape) noexcept -> bool
{
    if (_dstShape != dstShape) {
        _dstShape = dstShape;
        emit dstShapeChanged();
        emit styleModified();
        return true;
    }
    return false;
}

bool    EdgeStyle::setDashed(bool dashed) noexcept
{
    if (dashed != _dashed) {
        _dashed = dashed;
        emit dashedChanged();
        return true;
    }
    return false;
}

bool    EdgeStyle::setDashPattern(const QVector<qreal>& dashPattern) noexcept
{
    if (_dashPattern != dashPattern) {
        _dashPattern = dashPattern;
        emit dashPatternChanged();
        return true;
    }
    return false;
}

const QVector<qreal>& EdgeStyle::getDashPattern() const noexcept { return _dashPattern; }
//-----------------------------------------------------------------------------

} // ::qan

