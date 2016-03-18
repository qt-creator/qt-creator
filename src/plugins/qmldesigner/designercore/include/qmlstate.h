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

#include <qmldesignercorelib_global.h>
#include "qmlmodelnodefacade.h"
#include "qmlchangeset.h"

namespace QmlDesigner {

class AbstractViewAbstractVieweGroup;
class QmlObjectNode;
class QmlModelStateGroup;

class QMLDESIGNERCORE_EXPORT QmlModelState : public QmlModelNodeFacade
{
    friend class StatesEditorView;

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
    static bool isValidQmlModelState(const ModelNode &modelNode);
    void destroy();

    bool isBaseState() const;
    static bool isBaseState(const ModelNode &modelNode);
    QmlModelState duplicate(const QString &name) const;
    QmlModelStateGroup stateGroup() const;

    static ModelNode createQmlState(AbstractView *view, const PropertyListType &propertyList);

protected:
    void addChangeSetIfNotExists(const ModelNode &node);
    static QmlModelState createBaseState(const AbstractView *view);

};

} //QmlDesigner
