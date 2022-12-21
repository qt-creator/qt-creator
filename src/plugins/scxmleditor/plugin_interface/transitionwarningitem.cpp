// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "scxmleditortr.h"
#include "transitionitem.h"
#include "transitionwarningitem.h"

#include <utils/utilsicons.h>

using namespace ScxmlEditor::PluginInterface;

TransitionWarningItem::TransitionWarningItem(TransitionItem *parent)
    : WarningItem(parent)
    , m_parentItem(parent)
{
    setSeverity(OutputPane::Warning::WarningType);
    setTypeName(Tr::tr("Transition"));
    setDescription(Tr::tr("Transitions should be connected."));

    setPixmap(Utils::Icons::WARNING.pixmap());
}

void TransitionWarningItem::check()
{
    if (m_parentItem) {
        if (m_parentItem->targetType() == TransitionItem::ExternalNoTarget) {
            setReason(Tr::tr("Not connected (%1).").arg(m_parentItem->tagValue("event")));
            setWarningActive(true);
        } else
            setWarningActive(false);
    }
}
