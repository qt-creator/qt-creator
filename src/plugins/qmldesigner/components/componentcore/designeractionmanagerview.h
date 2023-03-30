// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <abstractview.h>
#include <selectioncontext.h>

#include "designeractionmanager.h"

namespace QmlDesigner {

class ActionInterface;

class DesignerActionManagerView : public AbstractView
{
    Q_OBJECT
public:
    DesignerActionManagerView(ExternalDependenciesInterface &externalDependencies);

    void modelAttached(Model *model) override;
    void modelAboutToBeDetached(Model *model) override;
    void nodeCreated(const ModelNode &) override;
    void nodeRemoved(const ModelNode &, const NodeAbstractProperty &, PropertyChangeFlags) override;
    void nodeAboutToBeReparented(const ModelNode &,
                                         const NodeAbstractProperty &,
                                         const NodeAbstractProperty &,
                                         AbstractView::PropertyChangeFlags ) override;
    void nodeReparented(const ModelNode &, const NodeAbstractProperty &,
                                const NodeAbstractProperty &,
                                AbstractView::PropertyChangeFlags) override;
    void propertiesRemoved(const QList<AbstractProperty>&) override;
    void rootNodeTypeChanged(const QString &, int , int ) override;
    void rewriterBeginTransaction() override;
    void rewriterEndTransaction() override;
    void currentStateChanged(const ModelNode &) override;
    void selectedNodesChanged(const QList<ModelNode> &,
                                      const QList<ModelNode> &) override;
    void nodeOrderChanged(const NodeListProperty &) override;
    void importsChanged(const Imports &, const Imports &) override;
    void signalHandlerPropertiesChanged(const QVector<SignalHandlerProperty> &/*propertyList*/, PropertyChangeFlags /*propertyChange*/) override;
    void variantPropertiesChanged(const QList<VariantProperty>& propertyList, PropertyChangeFlags propertyChangeFlag) override;
    void bindingPropertiesChanged(const QList<BindingProperty>& propertyList, PropertyChangeFlags propertyChangeFlag) override;

    DesignerActionManager &designerActionManager();
    const DesignerActionManager &designerActionManager() const;
    void emitSelectionChanged();
    void setupContext(SelectionContext::UpdateMode updateMode = SelectionContext::UpdateMode::Normal);

    void customNotification(const AbstractView *,
                            const QString &identifier,
                            const QList<ModelNode> &,
                            const QList<QVariant> &) override;
signals:
    void selectionChanged(bool itemsSelected, bool rootItemIsSelected);

private:
    DesignerActionManager m_designerActionManager;
    bool m_isInRewriterTransaction;
    bool m_setupContextDirty;
};


} // namespace QmlDesigner
