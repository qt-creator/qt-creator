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


#include "abstractaction.h"

namespace QmlDesigner {

AbstractAction::AbstractAction(const QString &description)
    : m_defaultAction(new DefaultAction(description))
{
}

AbstractAction::AbstractAction(DefaultAction *action)
    : m_defaultAction(action)
{
}

QAction *AbstractAction::action() const
{
    return m_defaultAction.data();
}

void AbstractAction::currentContextChanged(const SelectionContext &selectionContext)
{
    m_selectionContext = selectionContext;
    updateContext();
}

void AbstractAction::updateContext()
{
    m_defaultAction->setSelectionContext(m_selectionContext);
    if (m_selectionContext.isValid()) {
        m_defaultAction->setEnabled(isEnabled(m_selectionContext));
        m_defaultAction->setVisible(isVisible(m_selectionContext));
    }
}

DefaultAction *AbstractAction::defaultAction() const
{
    return m_defaultAction.data();
}

SelectionContext AbstractAction::selectionContext() const
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
