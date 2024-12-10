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
// \file	qanSelectable.cpp
// \author	benoit@destrat.io
// \date	2016 03 15
//-----------------------------------------------------------------------------

// QuickQanava headers
#include "./qanSelectable.h"
#include "./qanGraph.h"

namespace qan { // ::qan

/* Node Object Management *///-------------------------------------------------
Selectable::Selectable() { /* Nil */ }

Selectable::~Selectable() {
    if (_selectionItem &&  // Delete selection item if it has Cpp ownership
        QQmlEngine::objectOwnership(_selectionItem.data()) == QQmlEngine::CppOwnership)
        _selectionItem->deleteLater();
}

void    Selectable::configure(QQuickItem* target, qan::Graph* graph)
{
    _target = target;
    _graph = graph;
    if (_selectionItem)
        _selectionItem->setParentItem(_target);
}
//-----------------------------------------------------------------------------

/* Selection Management *///---------------------------------------------------
void    Selectable::setSelectable(bool selectable) noexcept
{
    if (_selectable != selectable) {
        _selectable = selectable;
        emitSelectableChanged();
    }

    // A selection item might have been created even if selectable property has not changed,
    // so for selection item update despite binding loop protection
    if (_selected &&
        !_selectable)
        setSelected(false);
}

void    Selectable::setSelected(bool selected) noexcept
{
    if (_target &&
        _graph) {  // Eventually create selection item
        if (selected &&
            getSelectionItem() == nullptr )
            setSelectionItem(_graph->createSelectionItem(_target.data()));
        else if (!selected)
            _graph->removeFromSelection(_target.data());
    }
    if (_selected != selected) {  // Binding loop protection
        _selected = selected;
        emitSelectedChanged();
    }
    if (getSelectionItem() != nullptr)    // Done outside of binding loop protection
        getSelectionItem()->setState(selected ? "SELECTED" : "UNSELECTED");
}

void    Selectable::setSelectionItem(QQuickItem* selectionItem) noexcept
{
    // PRECONITIONS:
        // selectionItem should not be nullptr, but no error/warning are reported
        // since a delegate creation might perfectly fail...
    if (selectionItem == nullptr)
        return;
    if (selectionItem != _selectionItem) {
        if (_selectionItem) {      // Clean the old selection item
            _selectionItem->setParentItem(nullptr); // Force QML garbage collection
            _selectionItem->setEnabled(false);      // Disable and hide item in case it is not
            _selectionItem->setVisible(false);      // immediately destroyed or garbage collected
            if (QQmlEngine::objectOwnership(_selectionItem.data()) == QQmlEngine::CppOwnership)
                _selectionItem->deleteLater();
        }

        if (selectionItem) {
            _selectionItem = QPointer<QQuickItem>(selectionItem);
            if (_selectionItem) {
                if (getSelectable())
                    _selectionItem->setState(getSelected() ? "SELECTED" : "UNSELECTED");
                if (_target) {
                    _selectionItem->setParentItem(_target.data());  // Configure Quick item
                    _selectionItem->setZ(1.0);
                }
            }
        }

        // Configure item and notify change
        configureSelectionItem();
        emitSelectionItemChanged();
    }
}

void    Selectable::configureSelectionItem()
{
    if (_target &&
        _selectionItem &&
        _graph) {
        // If selection item support QuickQanava selection interface, try to configure properties directly
        const auto selectionColorProperty = _selectionItem->property("selectionColor");
        if (selectionColorProperty.isValid())
            _selectionItem->setProperty("selectionColor", QVariant::fromValue(_graph->getSelectionColor()));
        const auto selectionWeightProperty = _selectionItem->property("selectionWeight");
        if (selectionWeightProperty.isValid())
            _selectionItem->setProperty("selectionWeight", QVariant::fromValue(_graph->getSelectionWeight()));
        const auto selectionMarginProperty = _selectionItem->property("selectionWeight");
        if (selectionMarginProperty.isValid())
            _selectionItem->setProperty("selectionMargin", QVariant::fromValue(_graph->getSelectionMargin()));

        const auto selectionMargin = _graph->getSelectionMargin();
        const auto selectionWeight = _graph->getSelectionWeight();

        const qreal x = -(selectionWeight / 2. + selectionMargin);
        const qreal y = -(selectionWeight / 2. + selectionMargin);
        const qreal width = _target->width() + selectionWeight + (selectionMargin * 2);
        const qreal height = _target->height() + selectionWeight + (selectionMargin * 2);

        _selectionItem->setX(x);
        _selectionItem->setY(y);
        _selectionItem->setZ(1.0);
        _selectionItem->setWidth(width);
        _selectionItem->setHeight(height);
        _selectionItem->setVisible(true);
    }
}
//-----------------------------------------------------------------------------

} // ::qan
