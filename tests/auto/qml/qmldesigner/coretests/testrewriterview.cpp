/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

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
