// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "aitransaction.h"

#include "aiassistantutils.h"
#include "airesponse.h"

#include <modelnode.h>
#include <qmldesignerplugin.h>

#include <qmljs/qmljsdocument.h>

namespace QmlDesigner {

AiTransaction::AiTransaction() = default;

AiTransaction::AiTransaction(const AiResponse &response)
{
    QStringList selectedIds = response.selectedIds();
    if (!selectedIds.isEmpty()) {
        m_before.selectedItems = AiAssistantUtils::currentSelectedIds();
        m_after.selectedItems = selectedIds;
        m_affectsIds = m_before.selectedItems != m_after.selectedItems;
        m_isValid = true;
    }

    const QString newQml = AiAssistantUtils::reformatQml(response.qml());
    if (!newQml.isEmpty()) {
        const QString currentQml = AiAssistantUtils::currentQmlText();
        m_before.qml = currentQml;
        m_after.qml = newQml;
        if (m_before.qml != m_after.qml) {
            m_producesQmlError = !AiAssistantUtils::isValidQmlCode(m_after.qml);
            m_affectsQml = true;
            m_isValid = true;
        }
    }
}

void AiTransaction::commit()
{
    if (!isValid() || applied())
        return;

    if (m_affectsIds) {
        AiAssistantUtils::selectIds(m_after.selectedItems);
        m_applied = true;
    }

    if (m_affectsQml) {
        AiAssistantUtils::setQml(m_after.qml);
        m_applied = true;
    }
}

void AiTransaction::rollback()
{
    if (!isValid() || !applied())
        return;

    if (m_affectsIds)
        AiAssistantUtils::selectIds(m_before.selectedItems);

    if (m_affectsQml)
        AiAssistantUtils::setQml(m_before.qml);

    m_applied = false;
}

} // namespace QmlDesigner
