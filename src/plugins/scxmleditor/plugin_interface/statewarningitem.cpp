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

#include "statewarningitem.h"
#include "idwarningitem.h"

#include <utils/utilsicons.h>

using namespace ScxmlEditor::PluginInterface;

StateWarningItem::StateWarningItem(StateItem *parent)
    : WarningItem(parent)
{
    setSeverity(OutputPane::Warning::InfoType);
    setTypeName(tr("State"));
    setDescription(tr("Draw some transitions to state."));

    setPixmap(Utils::Icons::WARNING.pixmap());
    setReason(tr("No input connection"));
}

void StateWarningItem::setIdWarning(IdWarningItem *idwarning)
{
    m_idWarningItem = idwarning;
}

void StateWarningItem::check()
{
    if (m_parentItem) {
        if (m_idWarningItem && m_idWarningItem->isVisible())
            setWarningActive(false);
        else {
            bool outputProblem = !m_parentItem->hasOutputTransitions(m_parentItem, true);
            bool inputProblem = !m_parentItem->isInitial() && !m_parentItem->hasInputTransitions(m_parentItem, true);

            if (outputProblem && inputProblem) {
                setReason(tr("No input or output connections (%1)").arg(m_parentItem->itemId()));
                setDescription(tr("Draw some transitions to or from state."));
                setWarningActive(true);
            } else if (outputProblem) {
                setReason(tr("No output connections (%1)").arg(m_parentItem->itemId()));
                setDescription(tr("Draw some transitions from state."));
                setWarningActive(true);
            } else if (inputProblem) {
                setReason(tr("No input connections (%1)").arg(m_parentItem->itemId()));
                setDescription(tr("Draw some transitions to state."));
                setWarningActive(true);
            } else
                setWarningActive(false);
        }
    }
}
