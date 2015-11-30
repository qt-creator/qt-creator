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
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
****************************************************************************/

#ifndef DESIGNERACTIONMANAGER_H
#define DESIGNERACTIONMANAGER_H

#include <qmldesignercorelib_global.h>
#include "actioninterface.h"
#include "abstractview.h"

namespace QmlDesigner {

class DesignerActionManagerView;

class QMLDESIGNERCORE_EXPORT DesignerActionManager {
public:
    DesignerActionManager(DesignerActionManagerView *designerActionManagerView);
    ~DesignerActionManager();

    void addDesignerAction(ActionInterface *newAction);
    QList<ActionInterface* > designerActions() const;

    void createDefaultDesignerActions();
    AbstractView *view();

private:
    QList<QSharedPointer<ActionInterface> > m_designerActions;
    DesignerActionManagerView *m_designerActionManagerView;
};

} //QmlDesigner

#endif //DESIGNERACTIONMANAGER_H
