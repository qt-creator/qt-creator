// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "designeractionmanagerview.h"

#include <customnotifications.h>

#include <selectioncontext.h>
#include <actioninterface.h>
#include <variantproperty.h>

namespace QmlDesigner {

DesignerActionManagerView::DesignerActionManagerView(ExternalDependenciesInterface &externalDependencies)
    : AbstractView{externalDependencies}
    , m_designerActionManager(this, externalDependencies)
    , m_isInRewriterTransaction(false)
    , m_setupContextDirty(false)
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
    setupContext(SelectionContext::UpdateMode::NodeCreated);
}

void DesignerActionManagerView::nodeRemoved(const ModelNode &, const NodeAbstractProperty &, AbstractView::PropertyChangeFlags)
{
    setupContext();
}

void DesignerActionManagerView::nodeAboutToBeReparented(const ModelNode &, const NodeAbstractProperty &, const NodeAbstractProperty &, AbstractView::PropertyChangeFlags)
{
    setupContext(SelectionContext::UpdateMode::NodeHierachy);
}

void DesignerActionManagerView::nodeReparented(const ModelNode &, const NodeAbstractProperty &, const NodeAbstractProperty &, AbstractView::PropertyChangeFlags)
{
    setupContext(SelectionContext::UpdateMode::NodeHierachy);
}

void DesignerActionManagerView::propertiesRemoved(const QList<AbstractProperty> &)
{
    setupContext(SelectionContext::UpdateMode::Properties);
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

void DesignerActionManagerView::nodeOrderChanged(const NodeListProperty &)
{
    setupContext(SelectionContext::UpdateMode::NodeHierachy);
}

void DesignerActionManagerView::importsChanged(const Imports &, const Imports &)
{
    setupContext();
}

void DesignerActionManagerView::signalHandlerPropertiesChanged(const QVector<SignalHandlerProperty> &, AbstractView::PropertyChangeFlags)
{
    setupContext(SelectionContext::UpdateMode::Properties);
}

void DesignerActionManagerView::variantPropertiesChanged(const QList<VariantProperty> &, AbstractView::PropertyChangeFlags propertyChangeFlag)
{
    if (propertyChangeFlag == AbstractView::PropertiesAdded)
        setupContext(SelectionContext::UpdateMode::Properties);
    else if (hasSingleSelectedModelNode())
        setupContext(SelectionContext::UpdateMode::Properties);
}

void DesignerActionManagerView::bindingPropertiesChanged(const QList<BindingProperty> &, AbstractView::PropertyChangeFlags propertyChangeFlag)
{
    if (propertyChangeFlag == AbstractView::PropertiesAdded)
        setupContext(SelectionContext::UpdateMode::Properties);
}

void DesignerActionManagerView::customNotification(const AbstractView * /*view*/,
                                      const QString &identifier,
                                      const QList<ModelNode> & /* nodeList */,
                                      const QList<QVariant> & /*data */)
{
    if (identifier == StartRewriterAmend)
        m_isInRewriterTransaction = true;
    else if (identifier == EndRewriterAmend)
        m_isInRewriterTransaction = false;
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
    const QList<ActionInterface *> actions = m_designerActionManager.designerActions();
    for (ActionInterface *action : actions) {
        action->currentContextChanged(selectionContext);
    }
    m_setupContextDirty = false;
}


} // namespace QmlDesigner
