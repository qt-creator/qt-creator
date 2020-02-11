/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include "view3dactioncommand.h"

#include <abstractaction.h>

#include <QtWidgets/qaction.h>
#include <QtGui/qicon.h>

namespace QmlDesigner {

using SelectionContextOperation = std::function<void(const SelectionContext &)>;

class Edit3DActionTemplate : public DefaultAction
{

public:
    Edit3DActionTemplate(const QString &description, SelectionContextOperation action,
                         View3DActionCommand::Type type);

    void actionTriggered(bool b) override;

    SelectionContextOperation m_action;
    View3DActionCommand::Type m_type;
};

class Edit3DAction : public AbstractAction
{
public:
    Edit3DAction(const QByteArray &menuId, View3DActionCommand::Type type,
                 const QString &description, const QKeySequence &key, bool checkable, bool checked,
                 const QIcon &iconOff, const QIcon &iconOn,
                 SelectionContextOperation selectionAction = nullptr);

    QByteArray category() const override;

    int priority() const override
    {
        return CustomActionsPriority;
    }

    Type type() const override
    {
        return ActionInterface::Edit3DAction;
    }

    QByteArray menuId() const override
    {
        return m_menuId;
    }

protected:
    bool isVisible(const SelectionContext &selectionContext) const override;
    bool isEnabled(const SelectionContext &selectionContext) const override;

private:
    QByteArray m_menuId;
};

} // namespace QmlDesigner
