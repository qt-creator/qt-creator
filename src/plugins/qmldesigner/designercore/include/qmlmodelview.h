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

#ifndef QMLMODELVIEW_H
#define QMLMODELVIEW_H

#include <qmldesignercorelib_global.h>
#include <abstractview.h>
#include "qmlitemnode.h"
#include "qmlstate.h"
#include "nodeinstanceview.h"


namespace QmlDesigner {

class ItemLibraryEntry;

class QMLDESIGNERCORE_EXPORT QmlModelView : public AbstractView
{
    Q_OBJECT
    friend class QmlObjectNode;
    friend class QmlModelNodeFacade;

public:
    QmlModelView(QObject *parent) ;

    void setCurrentState(const QmlModelState &state);
    QmlModelState currentState() const;

    QmlModelState baseState() const;
    QmlModelStateGroup rootStateGroup() const;

    QmlItemNode rootQmlItemNode() const;

protected:
    NodeInstance instanceForModelNode(const ModelNode &modelNode);
    bool hasInstanceForModelNode(const ModelNode &modelNode);

    void activateState(const QmlModelState &state);

};

} //QmlDesigner

#endif // QMLMODELVIEW_H
