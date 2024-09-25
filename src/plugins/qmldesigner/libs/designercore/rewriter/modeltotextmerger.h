// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <abstractview.h>
#include "rewriteaction.h"

namespace QmlDesigner {

class RewriterView;

namespace Internal {

class ModelToTextMerger
{
    using PropertyChangeFlags = AbstractView::PropertyChangeFlags;

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

    void addImports(const Imports &import);
    void removeImports(const Imports &imports);

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

    void dumpRewriteActions(QStringView msg);

private:
    RewriterView *m_rewriterView;
    QList<RewriteAction *> m_rewriteActions;
};

} //Internal
} //QmlDesigner
