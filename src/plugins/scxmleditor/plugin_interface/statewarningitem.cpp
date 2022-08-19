// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

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
    setReason(tr("No input connection."));
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
                setReason(tr("No input or output connections (%1).").arg(m_parentItem->itemId()));
                setDescription(tr("Draw some transitions to or from state."));
                setWarningActive(true);
            } else if (outputProblem) {
                setReason(tr("No output connections (%1).").arg(m_parentItem->itemId()));
                setDescription(tr("Draw some transitions from state."));
                setWarningActive(true);
            } else if (inputProblem) {
                setReason(tr("No input connections (%1).").arg(m_parentItem->itemId()));
                setDescription(tr("Draw some transitions to state."));
                setWarningActive(true);
            } else
                setWarningActive(false);
        }
    }
}
