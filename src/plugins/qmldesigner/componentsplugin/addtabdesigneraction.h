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

private:
    void addNewTab();
};

} // namespace QmlDesigner
