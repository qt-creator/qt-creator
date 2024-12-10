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
// \file	qanStyleManager.cpp
// \author	benoit@destrat.io
// \date	2015 06 05
//-----------------------------------------------------------------------------

// Qt headers
#include <QFont>
#include <QQuickItemGrabResult>
#include <QQmlEngine>

// QuickQanava headers
#include "./qanGraph.h"
#include "./qanStyleManager.h"

namespace qan { // ::qan

/* Style Object Management *///------------------------------------------------
StyleManager::StyleManager(QObject* parent) :
    QObject{parent}
{
}

StyleManager::~StyleManager() { /* Style manager does not own styles */ }

void    StyleManager::clear()
{
    _styles.clear();
    _nodeStyles.clear();
    _edgeStyles.clear();
}
//-----------------------------------------------------------------------------

/* Style Management *///-------------------------------------------------------
void    StyleManager::setStyleComponent(qan::Style* style, QQmlComponent* component) noexcept
{
    if (style != nullptr &&
        component != nullptr) {
        _styleComponentMap.insert(style, component);
        if (!_styles.contains(style))
            _styles.append(style);
    }
}

QQmlComponent*  StyleManager::getStyleComponent(qan::Style* style) noexcept
{
    if ( style == nullptr )
        return nullptr;
    auto component = _styleComponentMap.value(style, nullptr);
    if (component)
        QQmlEngine::setObjectOwnership(component.data(), QQmlEngine::CppOwnership);
    return component.data();
}

void    StyleManager::setNodeStyle( QQmlComponent* delegate, qan::NodeStyle* nodeStyle )
{
    if ( delegate != nullptr &&
         nodeStyle != nullptr )
        _nodeStyles.insert( delegate, nodeStyle );
}

qan::NodeStyle* StyleManager::getNodeStyle( QQmlComponent* delegate )
{
    if ( delegate != nullptr &&
         _nodeStyles.contains( delegate ) )
        return _nodeStyles.value( delegate, nullptr );

    return nullptr;
}

void    StyleManager::setEdgeStyle( QQmlComponent* delegate, qan::EdgeStyle* edgeStyle )
{
    if ( delegate != nullptr &&
         edgeStyle != nullptr )
        _edgeStyles.insert( delegate, edgeStyle );
}

qan::EdgeStyle* StyleManager::getEdgeStyle( QQmlComponent* delegate )
{
    if ( delegate != nullptr &&
         _edgeStyles.contains( delegate ) )
        return _edgeStyles.value( delegate, nullptr );
    return nullptr;
}

qan::EdgeStyle* StyleManager::createEdgeStyle()
{
    const auto edgeStyle = new qan::EdgeStyle{};
    _styles.push_back(edgeStyle);
    return edgeStyle;
}

qan::Style*     StyleManager::getStyleAt( int s )
{
    if ( s < _styles.size() ) {
        const auto style = qobject_cast<qan::Style*>(_styles.at(s));
        if ( style != nullptr )
            QQmlEngine::setObjectOwnership(style, QQmlEngine::CppOwnership );
        return style;
    }
    return nullptr;
}
//-----------------------------------------------------------------------------

} // ::qan

