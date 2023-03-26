// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "squishmessages.h"

#include "squishtr.h"

#include <coreplugin/icore.h>

namespace Squish::Internal::SquishMessages {

void criticalMessage(const QString &title, const QString &details)
{
    QMessageBox::critical(Core::ICore::dialogParent(), title, details);
}

void criticalMessage(const QString &details)
{
    criticalMessage(Tr::tr("Error"), details);
}

void toolsInUnexpectedState(int state, const QString &additionalDetails)
{
    QString details = Tr::tr("Squish Tools in unexpected state (%1).").arg(state);
    if (!additionalDetails.isEmpty())
        details.append('\n').append(additionalDetails);
    criticalMessage(details);
}

QMessageBox::StandardButton simpleQuestion(const QString &title, const QString &detail)
{
    return QMessageBox::question(Core::ICore::dialogParent(), title, detail);
}

} // Squish::Internal::SquishMessages
