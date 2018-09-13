/****************************************************************************
**
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

#include "designeractionmanagerview.h"

#include <selectioncontext.h>
#include <actioninterface.h>
#include <variantproperty.h>

namespace QmlDesigner {

DesignerActionManagerView::DesignerActionManagerView()
    : AbstractView(nullptr),
      m_designerActionManager(this),
      m_isInRewriterTransaction(false),
      m_setupContextDirty(false)
{
}

void DesignerActionManagerView::modelAttached(Model *model)
{
    AbstractView::modelAttached(model);
    setupContext();
}

void DesignerActionManagerView::modelAboutToBeDetached(Model *model)
{
    AbstractView::modelAboutToBeDetached(model);
    setupContext();
}

void DesignerActionManagerView::nodeCreated(const ModelNode &)
{
    setupContext(SelectionContext::UpdateMode::Fast);
}

void DesignerActionManagerView::nodeRemoved(const ModelNode &, const NodeAbstractProperty &, AbstractView::PropertyChangeFlags)
{
    setupContext();
}

void DesignerActionManagerView::nodeAboutToBeReparented(const ModelNode &, const NodeAbstractProperty &, const NodeAbstractProperty &, AbstractView::PropertyChangeFlags)
{
    setupContext(SelectionContext::UpdateMode::Fast);
}

void DesignerActionManagerView::nodeReparented(const ModelNode &, const NodeAbstractProperty &, const NodeAbstractProperty &, AbstractView::PropertyChangeFlags)
{
    setupContext(SelectionContext::UpdateMode::Fast);
}

void DesignerActionManagerView::propertiesRemoved(const QList<AbstractProperty> &)
{
    setupContext(SelectionContext::UpdateMode::Fast);
}

void DesignerActionManagerView::rootNodeTypeChanged(const QString &, int, int)
{
    setupContext();
}

void DesignerActionManagerView::rewriterBeginTransaction()
{
    m_isInRewriterTransaction = true;
}

void DesignerActionManagerView::rewriterEndTransaction()
{
    m_isInRewriterTransaction = false;

    if (m_setupContextDirty)
        setupContext();
}

void DesignerActionManagerView::currentStateChanged(const ModelNode &)
{
    setupContext(SelectionContext::UpdateMode::Fast);
}

void DesignerActionManagerView::selectedNodesChanged(const QList<ModelNode> &selectedNodes, const QList<ModelNode> &)
{
    setupContext(SelectionContext::UpdateMode::Fast);

    /* This breaks encapsulation, but the selection state is a very minor information.
     * Without this signal the ShortcutManager would have to be refactored completely.
     * This signal is only used to update the state of the cut/copy/delete actions.
    */
    emit selectionChanged(!selectedNodes.isEmpty(), singleSelectedModelNode().isRootNode());
}

void DesignerActionManagerView::nodeOrderChanged(const NodeListProperty &, const ModelNode &, int)
{
    setupContext(SelectionContext::UpdateMode::Fast);
}

void DesignerActionManagerView::importsChanged(const QList<Import> &, const QList<Import> &)
{
    setupContext();
}

void DesignerActionManagerView::signalHandlerPropertiesChanged(const QVector<SignalHandlerProperty> &, AbstractView::PropertyChangeFlags)
{
    setupContext(SelectionContext::UpdateMode::Fast);
}

void DesignerActionManagerView::variantPropertiesChanged(const QList<VariantProperty> &, AbstractView::PropertyChangeFlags propertyChangeFlag)
{
    if (propertyChangeFlag == AbstractView::PropertiesAdded)
        setupContext(SelectionContext::UpdateMode::Fast);
    else if (hasSingleSelectedModelNode())
        setupContext(SelectionContext::UpdateMode::Fast);
}

void DesignerActionManagerView::bindingPropertiesChanged(const QList<BindingProperty> &, AbstractView::PropertyChangeFlags propertyChangeFlag)
{
    if (propertyChangeFlag == AbstractView::PropertiesAdded)
        setupContext(SelectionContext::UpdateMode::Fast);
}

void DesignerActionManagerView::instancePropertyChanged(const QList<QPair<ModelNode, PropertyName> > &)
{
    if (hasSingleSelectedModelNode())
        setupContext(SelectionContext::UpdateMode::Fast);
}

DesignerActionManager &DesignerActionManagerView::designerActionManager()
{
    return m_designerActionManager;
}

const DesignerActionManager &DesignerActionManagerView::designerActionManager() const
{
    return m_designerActionManager;
}

void DesignerActionManagerView::emitSelectionChanged()
{
    if (model())
        emit selectionChanged(!selectedModelNodes().isEmpty(), singleSelectedModelNode().isRootNode());
}

/* We should consider compressing this. */
/* One update every 100ms should be enough. */
void DesignerActionManagerView::setupContext(SelectionContext::UpdateMode updateMode)
{
    if (m_isInRewriterTransaction) {
        m_setupContextDirty = true;
        return;
    }
    SelectionContext selectionContext(this);
    selectionContext.setUpdateMode(updateMode);
    foreach (ActionInterface* action, m_designerActionManager.designerActions()) {
        action->currentContextChanged(selectionContext);
    }
    m_setupContextDirty = false;
}


} // namespace QmlDesigner
