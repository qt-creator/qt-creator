// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "aiassistanttermsdialog.h"

#include <utils/fileutils.h>

#include <QCheckBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>

namespace QmlDesigner {

AiAssistantTermsDialog::AiAssistantTermsDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("AI Assistant Terms and Conditions"));
    resize(500, 400);

    setLayout(new QVBoxLayout(this));

    auto *label = new QLabel(tr("Please read and accept the Terms and Conditions:"), this);
    layout()->addWidget(label);

    m_termsText = new QTextEdit(this);
    m_termsText->setReadOnly(true);
    m_termsText->setHtml(Utils::FileUtils::fetchQrc(":/AiAssistant/termsandconditions.html"));

    layout()->addWidget(m_termsText);

    m_agreeCheckbox = new QCheckBox(tr("I have read and agree to the Terms and Conditions"), this);
    layout()->addWidget(m_agreeCheckbox);
    connect(m_agreeCheckbox, &QCheckBox::stateChanged, this, [this](int state) {
        m_acceptButton->setEnabled(state == Qt::Checked);
    });

    auto *buttonLayout = new QHBoxLayout(this);
    m_acceptButton = new QPushButton(tr("Accept"), this);
    m_rejectButton = new QPushButton(tr("Reject"), this);

    m_acceptButton->setEnabled(false);

    connect(m_acceptButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(m_rejectButton, &QPushButton::clicked, this, &QDialog::reject);

    buttonLayout->addStretch();
    buttonLayout->addWidget(m_acceptButton);
    buttonLayout->addWidget(m_rejectButton);
    static_cast<QVBoxLayout *>(layout())->addLayout(buttonLayout);
}

} // namespace QmlDesigner
