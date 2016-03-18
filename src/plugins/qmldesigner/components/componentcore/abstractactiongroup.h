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

#include "actioninterface.h"

#include <QAction>
#include <QMenu>
#include <QScopedPointer>

namespace QmlDesigner {

class QMLDESIGNERCORE_EXPORT AbstractActionGroup : public ActionInterface
{
public:
    AbstractActionGroup(const QString &displayName);

    virtual bool isVisible(const SelectionContext &m_selectionState) const = 0;
    virtual bool isEnabled(const SelectionContext &m_selectionState) const = 0;
    ActionInterface::Type type() const override;
    QAction *action() const override;
    QMenu *menu() const;
    SelectionContext selectionContext() const;

    virtual void currentContextChanged(const SelectionContext &selectionContext) override;
    virtual void updateContext();

private:
    const QString m_displayName;
    SelectionContext m_selectionContext;
    QScopedPointer<QMenu> m_menu;
    QAction *m_action;
};

} // namespace QmlDesigner
