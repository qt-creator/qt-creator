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

#ifndef QMLDESIGNER_TABVIEWDESIGNERACTION_H
#define QMLDESIGNER_TABVIEWDESIGNERACTION_H

#include "abstractaction.h"

namespace QmlDesigner {

class AddTabDesignerAction : public QObject, public AbstractAction
{
    Q_OBJECT
public:
    AddTabDesignerAction();

    QByteArray category() const;
    QByteArray menuId() const;
    int priority() const;
    Type type() const;

protected:
    bool isVisible(const SelectionContext &selectionContext) const;
    bool isEnabled(const SelectionContext &selectionContext) const;

private slots:
    void addNewTab();
};

} // namespace QmlDesigner

#endif // QMLDESIGNER_TABVIEWDESIGNERACTION_H
