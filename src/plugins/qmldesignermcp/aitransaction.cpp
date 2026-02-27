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

    if (m_affectsQml) {
        AiAssistantUtils::setQml(m_after.qml);
        m_applied = true;
    }
}

void AiTransaction::rollback()
{
    if (!isValid() || !applied())
        return;

    if (m_affectsQml)
        AiAssistantUtils::setQml(m_before.qml);

    m_applied = false;
}

} // namespace QmlDesigner
