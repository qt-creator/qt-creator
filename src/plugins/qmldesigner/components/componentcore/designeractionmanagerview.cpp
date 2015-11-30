/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
****************************************************************************/

#include "designeractionmanagerview.h"

#include <selectioncontext.h>
#include <actioninterface.h>

namespace QmlDesigner {

DesignerActionManagerView::DesignerActionManagerView()
    : AbstractView(0),
      m_designerActionManager(this),
      m_isInRewriterTransaction(false),
      m_setupContextDirty(false)
{
    m_designerActionManager.createDefaultDesignerActions();
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
    setupContext();
}

void DesignerActionManagerView::nodeRemoved(const ModelNode &, const NodeAbstractProperty &, AbstractView::PropertyChangeFlags)
{
    setupContext();
}

void DesignerActionManagerView::nodeAboutToBeReparented(const ModelNode &, const NodeAbstractProperty &, const NodeAbstractProperty &, AbstractView::PropertyChangeFlags)
{
    setupContext();
}

void DesignerActionManagerView::nodeReparented(const ModelNode &, const NodeAbstractProperty &, const NodeAbstractProperty &, AbstractView::PropertyChangeFlags)
{
    setupContext();
}

void DesignerActionManagerView::propertiesRemoved(const QList<AbstractProperty> &)
{
    setupContext();
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
    setupContext();
}

void DesignerActionManagerView::selectedNodesChanged(const QList<ModelNode> &, const QList<ModelNode> &)
{
    setupContext();
}

void DesignerActionManagerView::nodeOrderChanged(const NodeListProperty &, const ModelNode &, int)
{
    setupContext();
}

void DesignerActionManagerView::importsChanged(const QList<Import> &, const QList<Import> &)
{
    setupContext();
}

void DesignerActionManagerView::setDesignerActionList(const QList<ActionInterface *> &designerActionList)
{
    m_designerActionList = designerActionList;
}

void DesignerActionManagerView::signalHandlerPropertiesChanged(const QVector<SignalHandlerProperty> &, AbstractView::PropertyChangeFlags)
{
    setupContext();
}

DesignerActionManager &DesignerActionManagerView::designerActionManager()
{
    return m_designerActionManager;
}

const DesignerActionManager &DesignerActionManagerView::designerActionManager() const
{
    return m_designerActionManager;
}

void DesignerActionManagerView::setupContext()
{
    if (m_isInRewriterTransaction) {
        m_setupContextDirty = true;
        return;
    }
    SelectionContext selectionContext(this);
    foreach (ActionInterface* action, m_designerActionList) {
        action->currentContextChanged(selectionContext);
    }
    m_setupContextDirty = false;
}


} // namespace QmlDesigner
