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

#ifndef MODELTOTEXTMERGER_H
#define MODELTOTEXTMERGER_H

#include "qmldesignercorelib_global.h"
#include <modelnode.h>
#include "abstractview.h"
#include "nodeabstractproperty.h"
#include "variantproperty.h"
#include "nodelistproperty.h"
#include "bindingproperty.h"
#include "rewriteaction.h"
#include <filemanager/qmlrefactoring.h>
#include <QMap>
#include <QSet>
#include <QHash>
#include <QVariant>

namespace QmlDesigner {

class RewriterView;

namespace Internal {

class ModelToTextMerger
{
    typedef AbstractView::PropertyChangeFlags PropertyChangeFlags;
    static QStringList m_propertyOrder;

public:
    ModelToTextMerger(RewriterView *reWriterView);

    /**
     *  Note: his method might throw exceptions, as the model works this way. So to
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

    void schedule(RewriteAction *action);
    QList<RewriteAction *> scheduledRewriteActions() const
    { return m_rewriteActions; }

    static QmlDesigner::QmlRefactoring::PropertyType propertyType(const AbstractProperty &property, const QString &textValue = QString());
    static QStringList getPropertyOrder();

    static bool isInHierarchy(const AbstractProperty &property);

    void dumpRewriteActions(const QString &msg);

private:
    RewriterView *m_rewriterView;
    QList<RewriteAction *> m_rewriteActions;
};

} //Internal
} //QmlDesigner

#endif // MODELTOTEXTMERGER_H
