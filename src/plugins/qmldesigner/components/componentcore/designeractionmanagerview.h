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

#pragma once

#include <abstractview.h>

#include "designeractionmanager.h"

namespace QmlDesigner {

class ActionInterface;

class DesignerActionManagerView : public AbstractView
{
    Q_OBJECT
public:
    DesignerActionManagerView();

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
    void nodeOrderChanged(const NodeListProperty &, const ModelNode &, int ) override;
    void importsChanged(const QList<Import> &, const QList<Import> &) override;
    void signalHandlerPropertiesChanged(const QVector<SignalHandlerProperty> &/*propertyList*/, PropertyChangeFlags /*propertyChange*/) override;
    void variantPropertiesChanged(const QList<VariantProperty>& propertyList, PropertyChangeFlags propertyChangeFlag) override;
    void bindingPropertiesChanged(const QList<BindingProperty>& propertyList, PropertyChangeFlags propertyChangeFlag) override;

    void instancePropertyChanged(const QList<QPair<ModelNode, PropertyName> > &propertyList) override;

    void setDesignerActionList(const QList<ActionInterface* > &designerActionList);
    DesignerActionManager &designerActionManager();
    const DesignerActionManager &designerActionManager() const;

signals:
    void selectionChanged(bool itemsSelected, bool rootItemIsSelected);

protected:
    void setupContext();

private:
    DesignerActionManager m_designerActionManager;
    QList<ActionInterface* > m_designerActionList;
    bool m_isInRewriterTransaction;
    bool m_setupContextDirty;
};


} // namespace QmlDesigner
