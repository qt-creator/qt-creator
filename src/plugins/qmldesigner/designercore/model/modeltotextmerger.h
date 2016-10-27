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
#include "rewriteaction.h"

namespace QmlDesigner {

class RewriterView;

namespace Internal {

class ModelToTextMerger
{
    typedef AbstractView::PropertyChangeFlags PropertyChangeFlags;

public:
    ModelToTextMerger(RewriterView *reWriterView);

    /**
     *  Note: his function might throw exceptions, as the model works this way. So to
     *  handle rewriting failures, you will also need to catch any exception coming
     *  out.
     */
    void applyChanges();

    bool hasNoPendingChanges() const
    { return m_rewriteActions.isEmpty(); }

    void nodeCreated(const ModelNode &createdNode);
    void nodeRemoved(const ModelNode &removedNode, const NodeAbstractProperty &parentProperty, PropertyChangeFlags propertyChange);
    void propertiesRemoved(const QList<AbstractProperty>& propertyList);
    void propertiesChanged(const QList<AbstractProperty>& propertyList, PropertyChangeFlags propertyChange);
    void nodeReparented(const ModelNode &node, const NodeAbstractProperty &newPropertyParent, const NodeAbstractProperty &oldPropertyParent, AbstractView::PropertyChangeFlags propertyChange);
    void nodeIdChanged(const ModelNode& node, const QString& newId, const QString& oldId);
    void nodeSlidAround(const ModelNode &movingNode, const ModelNode &inFrontOfNode);
    void nodeTypeChanged(const ModelNode &node,const QString &type, int majorVersion, int minorVersion);

    void addImport(const Import &import);
    void removeImport(const Import &import);

protected:
    RewriterView *view();

    void reindent(const QMap<int,int> &dirtyAreas) const;
    void reindentAll() const;

    void schedule(RewriteAction *action);
    QList<RewriteAction *> scheduledRewriteActions() const
    { return m_rewriteActions; }

    static QmlDesigner::QmlRefactoring::PropertyType propertyType(const AbstractProperty &property, const QString &textValue = QString());
    static PropertyNameList propertyOrder();

    static bool isInHierarchy(const AbstractProperty &property);

    void dumpRewriteActions(const QString &msg);

private:
    RewriterView *m_rewriterView;
    QList<RewriteAction *> m_rewriteActions;
};

} //Internal
} //QmlDesigner
