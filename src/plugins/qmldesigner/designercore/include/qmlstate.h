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

#ifndef FXSTATE_H
#define FXSTATE_H

#include <qmldesignercorelib_global.h>
#include "qmlmodelnodefacade.h"
#include "qmlchangeset.h"

namespace QmlDesigner {

class QmlModelView;
class QmlModelStateGroup;
class QmlObjectNode;

class QMLDESIGNERCORE_EXPORT QmlModelState : public QmlModelNodeFacade
{
    friend class QMLDESIGNERCORE_EXPORT QmlModelView;

public:
    QmlModelState();
    QmlModelState(const ModelNode &modelNode);

    QmlPropertyChanges propertyChanges(const ModelNode &node);
    QList<QmlModelStateOperation> stateOperations(const ModelNode &node) const;
    QList<QmlPropertyChanges> propertyChanges() const;
    QList<QmlModelStateOperation> stateOperations() const;

    bool hasPropertyChanges(const ModelNode &node) const;

    bool hasStateOperation(const ModelNode &node) const;

    void removePropertyChanges(const ModelNode &node);

    bool affectsModelNode(const ModelNode &node) const;
    QList<QmlObjectNode> allAffectedNodes() const;
    QString name() const;
    void setName(const QString &name);
    bool isValid() const;
    void destroy();

    bool isBaseState() const;
    QmlModelState duplicate(const QString &name) const;
    QmlModelStateGroup stateGroup() const;

protected:
    void addChangeSetIfNotExists(const ModelNode &node);
    static QmlModelState createBaseState(const QmlModelView *view);

};

} //QmlDesigner


#endif // FXSTATE_H
