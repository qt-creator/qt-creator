/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "testrewriterview.h"
#include <QObject>
#include <nodeproperty.h>

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
