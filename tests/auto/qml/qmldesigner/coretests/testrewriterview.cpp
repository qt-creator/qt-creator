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

#include "testrewriterview.h"
#include <QObject>
#include <nodeproperty.h>
#include <nodelistproperty.h>
#include <variantproperty.h>

using namespace QmlDesigner;
using namespace QmlDesigner::Internal;

bool TestModelToTextMerger::isNodeScheduledForRemoval(const ModelNode &node) const
{
    foreach (RewriteAction *action, scheduledRewriteActions()) {
        if (RemoveNodeRewriteAction *removeAction = action->asRemoveNodeRewriteAction()) {
            if (removeAction->node() == node)
                return true;
        }
    }

    return false;
}

bool TestModelToTextMerger::isNodeScheduledForAddition(const ModelNode &node) const
{
    foreach (RewriteAction *action, scheduledRewriteActions()) {
        if (AddPropertyRewriteAction *addPropertyAction = action->asAddPropertyRewriteAction()) {
            const AbstractProperty property = addPropertyAction->property();
            if (property.isNodeProperty() && property.toNodeProperty().modelNode() == node)
                return true;
            else if (property.isNodeListProperty() && property.toNodeListProperty().toModelNodeList().contains(node))
                return true;
        } else if (ChangePropertyRewriteAction *changePropertyAction = action->asChangePropertyRewriteAction()) {
            const AbstractProperty property = changePropertyAction->property();
            if (property.isNodeProperty() && property.toNodeProperty().modelNode() == node)
                return true;
            else if (property.isNodeListProperty() && property.toNodeListProperty().toModelNodeList().contains(node))
                return true;
        }

    }

    return false;
}

VariantProperty TestModelToTextMerger::findAddedVariantProperty(const VariantProperty &property) const
{
    foreach (RewriteAction *action, scheduledRewriteActions()) {
        if (AddPropertyRewriteAction *addPropertyAction = action->asAddPropertyRewriteAction()) {
            const AbstractProperty candidate = addPropertyAction->property();

            if (property.isVariantProperty() && property.toVariantProperty() == property)
                return property.toVariantProperty();
        }
    }

    return VariantProperty();
}

TestRewriterView::TestRewriterView(QObject *parent,
                                   DifferenceHandling differenceHandling)
    : RewriterView(differenceHandling, parent)
{
    //Unit tests do not like the semantic errors
    setCheckSemanticErrors(false);
}

bool TestRewriterView::isModificationGroupActive() const
{
    return RewriterView::isModificationGroupActive();
}

void TestRewriterView::setModificationGroupActive(bool active)
{
    RewriterView::setModificationGroupActive(active);
}

void TestRewriterView::applyModificationGroupChanges()
{
    RewriterView::applyModificationGroupChanges();
}

Internal::TestModelToTextMerger *TestRewriterView::modelToTextMerger() const
{
    return static_cast<Internal::TestModelToTextMerger*> (RewriterView::modelToTextMerger());
}

Internal::TextToModelMerger *TestRewriterView::textToModelMerger() const
{
    return RewriterView::textToModelMerger();
}
