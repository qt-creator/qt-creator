// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "perforcesubmiteditorwidget.h"

namespace Perforce {
namespace Internal {

PerforceSubmitEditorWidget::PerforceSubmitEditorWidget() :
    m_submitPanel(new QGroupBox)
{
    m_submitPanelUi.setupUi(m_submitPanel);
    insertTopWidget(m_submitPanel);
}

void PerforceSubmitEditorWidget::setData(const QString &change,
                                         const QString &client,
                                         const QString &userName)
{
    m_submitPanelUi.changeNumber->setText(change);
    m_submitPanelUi.clientName->setText(client);
    m_submitPanelUi.userName->setText(userName);
}

} // namespace Internal
} // namespace Perforce
