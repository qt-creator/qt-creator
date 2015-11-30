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

#ifndef ABSTRACTDESIGNERACTION_H
#define ABSTRACTDESIGNERACTION_H

#include "componentcore_constants.h"
#include "selectioncontext.h"

QT_BEGIN_NAMESPACE
class QAction;
QT_END_NAMESPACE

namespace QmlDesigner {

class QMLDESIGNERCORE_EXPORT ActionInterface
{
public:
    enum Type {
        Menu,
        Action
    };

    enum Priorities {
        HighestPriority = ComponentCoreConstants::priorityFirst,
        CustomActionsPriority = ComponentCoreConstants::priorityCustomActions,
        RefactoringActionsPriority = ComponentCoreConstants::priorityRefactoring,
        LowestPriority = ComponentCoreConstants::priorityLast
    };

    virtual ~ActionInterface() {}

    virtual QAction *action() const = 0;
    virtual QByteArray category() const = 0;
    virtual QByteArray menuId() const = 0;
    virtual int priority() const = 0;
    virtual Type type() const = 0;
    virtual void currentContextChanged(const SelectionContext &selectionState) = 0;

};

} //QmlDesigner

#endif //ABSTRACTDESIGNERACTION_H
