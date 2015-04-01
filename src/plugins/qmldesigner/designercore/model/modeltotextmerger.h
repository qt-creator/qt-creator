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
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
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
#include "bindingproperty.h"
#include "rewriteaction.h"
#include <filemanager/qmlrefactoring.h>
#include <QMap>

namespace QmlDesigner {

class RewriterView;

namespace Internal {

class ModelToTextMerger
{
    typedef AbstractView::PropertyChangeFlags PropertyChangeFlags;
    static PropertyNameList m_propertyOrder;

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
    static PropertyNameList getPropertyOrder();

    static bool isInHierarchy(const AbstractProperty &property);

    void dumpRewriteActions(const QString &msg);

private:
    RewriterView *m_rewriterView;
    QList<RewriteAction *> m_rewriteActions;
};

} //Internal
} //QmlDesigner

#endif // MODELTOTEXTMERGER_H
