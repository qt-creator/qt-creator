/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef FXSTATE_H
#define FXSTATE_H

#include <corelib_global.h>
#include "qmlmodelnodefacade.h"
#include "qmlchangeset.h"

namespace QmlDesigner {

class QmlModelView;
class QmlModelStateGroup;

class CORESHARED_EXPORT QmlModelState : public QmlModelNodeFacade
{
    friend class CORESHARED_EXPORT QmlModelView;

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

private:
    bool m_isBaseState;
};

} //QmlDesigner


#endif // FXSTATE_H
