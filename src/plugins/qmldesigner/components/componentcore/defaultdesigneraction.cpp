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


#include "defaultdesigneraction.h"

namespace QmlDesigner {

DefaultDesignerAction::DefaultDesignerAction(const QString &description)
    : m_defaultAction(new DefaultAction(description))
{
}

DefaultDesignerAction::DefaultDesignerAction(DefaultAction *action)
    : m_defaultAction(action)
{
}

QAction *DefaultDesignerAction::action() const
{
    return m_defaultAction.data();
}

void DefaultDesignerAction::currentContextChanged(const SelectionContext &selectionContext)
{
    m_selectionContext = selectionContext;
    updateContext();
}

void DefaultDesignerAction::updateContext()
{
    m_defaultAction->setSelectionContext(m_selectionContext);
    if (m_selectionContext.isValid()) {
        m_defaultAction->setEnabled(isEnabled(m_selectionContext));
        m_defaultAction->setVisible(isVisible(m_selectionContext));
    }
}

DefaultAction *DefaultDesignerAction::defaultAction() const
{
    return m_defaultAction.data();
}

SelectionContext DefaultDesignerAction::selectionContext() const
{
    return m_selectionContext;
}

DefaultAction::DefaultAction(const QString &description)
    : QAction(description, 0)
{
    connect(this, SIGNAL(triggered(bool)), this, SLOT(actionTriggered(bool)));
}

void DefaultAction::actionTriggered(bool enable)
{
    emit triggered(enable, m_selectionContext);
}

void DefaultAction::setSelectionContext(const SelectionContext &selectionContext)
{
    m_selectionContext = selectionContext;
}

} // namespace QmlDesigner
