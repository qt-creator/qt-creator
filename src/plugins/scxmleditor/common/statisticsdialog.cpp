// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "scxmldocument.h"
#include "statisticsdialog.h"

using namespace ScxmlEditor::Common;

StatisticsDialog::StatisticsDialog(QWidget *parent)
    : QDialog(parent)
{
    m_ui.setupUi(this);
    setWindowTitle(tr("Document Statistics"));
    connect(m_ui.m_okButton, &QPushButton::clicked, this, &StatisticsDialog::accept);
}

void StatisticsDialog::setDocument(ScxmlEditor::PluginInterface::ScxmlDocument *doc)
{
    m_ui.m_statistics->setDocument(doc);
}
