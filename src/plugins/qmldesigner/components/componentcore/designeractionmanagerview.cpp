// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "designeractionmanagerview.h"
#include "componentcoretracing.h"

#include <customnotifications.h>

#include <selectioncontext.h>
#include <actioninterface.h>
#include <variantproperty.h>

namespace QmlDesigner {

using ComponentCoreTracing::category;
using NanotraceHR::keyValue;

DesignerActionManagerView::DesignerActionManagerView(ExternalDependenciesInterface &externalDependencies,
                                                     ModulesStorage &modulesStorage)
    : AbstractView{externalDependencies}
    , m_designerActionManager(this, externalDependencies, modulesStorage)
    , m_isInRewriterTransaction(false)
    , m_setupContextDirty(false)
{
    NanotraceHR::Tracer tracer{"designer action manager view constructor", category()};
}

void DesignerActionManagerView::modelAttached(Model *model)
{
    NanotraceHR::Tracer tracer{"designer action manager view model attached", category()};

    AbstractView::modelAttached(model);
    setupContext();
}

void DesignerActionManagerView::modelAboutToBeDetached(Model *model)
{
    NanotraceHR::Tracer tracer{"designer action manager view model about to be detached", category()};

    AbstractView::modelAboutToBeDetached(model);
    setupContext();
}

void DesignerActionManagerView::nodeCreated(const ModelNode &)
{
    NanotraceHR::Tracer tracer{"designer action manager view node created", category()};

    setupContext(SelectionContext::UpdateMode::NodeCreated);
}

void DesignerActionManagerView::nodeRemoved(const ModelNode &, const NodeAbstractProperty &, AbstractView::PropertyChangeFlags)
{
    NanotraceHR::Tracer tracer{"designer action manager view node removed", category()};

    setupContext();
}

void DesignerActionManagerView::nodeAboutToBeReparented(const ModelNode &, const NodeAbstractProperty &, const NodeAbstractProperty &, AbstractView::PropertyChangeFlags)
{
    NanotraceHR::Tracer tracer{"designer action manager view node about to be reparented", category()};

    setupContext(SelectionContext::UpdateMode::NodeHierachy);
}

void DesignerActionManagerView::nodeReparented(const ModelNode &, const NodeAbstractProperty &, const NodeAbstractProperty &, AbstractView::PropertyChangeFlags)
{
    NanotraceHR::Tracer tracer{"designer action manager view node reparented", category()};

    setupContext(SelectionContext::UpdateMode::NodeHierachy);
}

void DesignerActionManagerView::propertiesRemoved(const QList<AbstractProperty> &)
{
    NanotraceHR::Tracer tracer{"designer action manager view properties removed", category()};

    setupContext(SelectionContext::UpdateMode::Properties);
}

void DesignerActionManagerView::rootNodeTypeChanged(const QString &)
{
    NanotraceHR::Tracer tracer{"designer action manager view root node type changed", category()};

    setupContext();
}

void DesignerActionManagerView::rewriterBeginTransaction()
{
    NanotraceHR::Tracer tracer{"designer action manager view rewriter begin transaction", category()};

    m_isInRewriterTransaction = true;
}

void DesignerActionManagerView::rewriterEndTransaction()
{
    NanotraceHR::Tracer tracer{"designer action manager view rewriter end transaction", category()};

    m_isInRewriterTransaction = false;

    if (m_setupContextDirty)
        setupContext();
}

void DesignerActionManagerView::currentStateChanged(const ModelNode &)
{
    NanotraceHR::Tracer tracer{"designer action manager view current state changed", category()};

    setupContext(SelectionContext::UpdateMode::Fast);
}

void DesignerActionManagerView::selectedNodesChanged(const QList<ModelNode> &selectedNodes, const QList<ModelNode> &)
{
    NanotraceHR::Tracer tracer{"designer action manager view selected nodes changed", category()};

    setupContext(SelectionContext::UpdateMode::Fast);

    /* This breaks encapsulation, but the selection state is a very minor information.
     * Without this signal the ShortcutManager would have to be refactored completely.
     * This signal is only used to update the state of the cut/copy/delete actions.
    */
    emit selectionChanged(!selectedNodes.isEmpty(), singleSelectedModelNode().isRootNode());
}

void DesignerActionManagerView::nodeOrderChanged(const NodeListProperty &)
{
    NanotraceHR::Tracer tracer{"designer action manager view node order changed", category()};

    setupContext(SelectionContext::UpdateMode::NodeHierachy);
}

void DesignerActionManagerView::importsChanged(const Imports &, const Imports &)
{
    NanotraceHR::Tracer tracer{"designer action manager view imports changed", category()};

    setupContext();
}

void DesignerActionManagerView::signalHandlerPropertiesChanged(const QVector<SignalHandlerProperty> &, AbstractView::PropertyChangeFlags)
{
    NanotraceHR::Tracer tracer{"designer action manager view signal handler properties changed",
                               category()};

    setupContext(SelectionContext::UpdateMode::Properties);
}

void DesignerActionManagerView::variantPropertiesChanged(const QList<VariantProperty> &, AbstractView::PropertyChangeFlags propertyChangeFlag)
{
    NanotraceHR::Tracer tracer{"designer action manager view variant properties changed", category()};

    if (propertyChangeFlag == AbstractView::PropertiesAdded)
        setupContext(SelectionContext::UpdateMode::Properties);
    else if (hasSingleSelectedModelNode())
        setupContext(SelectionContext::UpdateMode::Properties);
}

void DesignerActionManagerView::bindingPropertiesChanged(const QList<BindingProperty> &, AbstractView::PropertyChangeFlags propertyChangeFlag)
{
    NanotraceHR::Tracer tracer{"designer action manager view binding properties changed", category()};

    if (propertyChangeFlag == AbstractView::PropertiesAdded)
        setupContext(SelectionContext::UpdateMode::Properties);
}

void DesignerActionManagerView::customNotification(const AbstractView * /*view*/,
                                      const QString &identifier,
                                      const QList<ModelNode> & /* nodeList */,
                                      const QList<QVariant> & /*data */)
{
    NanotraceHR::Tracer tracer{"designer action manager view custom notification", category()};

    if (identifier == StartRewriterAmend)
        m_isInRewriterTransaction = true;
    else if (identifier == EndRewriterAmend)
        m_isInRewriterTransaction = false;
}

DesignerActionManager &DesignerActionManagerView::designerActionManager()
{
    NanotraceHR::Tracer tracer{"designer action manager view designer action manager", category()};

    return m_designerActionManager;
}

const DesignerActionManager &DesignerActionManagerView::designerActionManager() const
{
    NanotraceHR::Tracer tracer{"designer action manager view designer action manager const",
                               category()};

    return m_designerActionManager;
}

void DesignerActionManagerView::emitSelectionChanged()
{
    NanotraceHR::Tracer tracer{"designer action manager view emit selection changed", category()};

    if (model())
        emit selectionChanged(!selectedModelNodes().isEmpty(), singleSelectedModelNode().isRootNode());
}

/* We should consider compressing this. */
/* One update every 100ms should be enough. */
void DesignerActionManagerView::setupContext(SelectionContext::UpdateMode updateMode)
{
    NanotraceHR::Tracer tracer{"designer action manager view setup context", category()};

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
