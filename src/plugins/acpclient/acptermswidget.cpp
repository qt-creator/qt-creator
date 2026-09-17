// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "acptermswidget.h"

#include "acpclienttr.h"
#include "acpsettings.h"

#include <utils/qtdesignwidgets.h>
#include <utils/stylehelper.h>

#include <QAbstractButton>
#include <QCheckBox>
#include <QFile>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QTextBrowser>
#include <QVBoxLayout>

namespace AcpClient::Internal {

static QString termsText()
{
    QFile file(":/acpclient/terms/terms-and-conditions.md");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return {};
    return QString::fromUtf8(file.readAll());
}

// Makes the confirmation text part of the check box's click target, which a plain QLabel
// next to it would not be.
class ConfirmationLabel : public QLabel
{
public:
    ConfirmationLabel(const QString &text, QAbstractButton *button, QWidget *parent)
        : QLabel(text, parent)
        , m_button(button)
    {
        setWordWrap(true);
    }

protected:
    void mousePressEvent(QMouseEvent *event) override
    {
        if (event->button() == Qt::LeftButton)
            event->accept();
        else
            QLabel::mousePressEvent(event);
    }

    void mouseReleaseEvent(QMouseEvent *event) override
    {
        if (event->button() == Qt::LeftButton && rect().contains(event->position().toPoint()))
            m_button->toggle();
        else
            QLabel::mouseReleaseEvent(event);
    }

private:
    QAbstractButton *m_button;
};

AcpTermsWidget::AcpTermsWidget(QWidget *parent)
    : QWidget(parent)
{
    using Utils::StyleHelper::SpacingTokens::GapHM;
    using Utils::StyleHelper::SpacingTokens::GapHS;
    using Utils::StyleHelper::SpacingTokens::GapVM;
    using Utils::StyleHelper::SpacingTokens::PaddingHL;
    using Utils::StyleHelper::SpacingTokens::PaddingVL;

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(PaddingHL, PaddingVL, PaddingHL, PaddingVL);
    layout->setSpacing(GapVM);

    auto *title = new QLabel(Tr::tr("Terms and Conditions"), this);
    title->setFont(Utils::StyleHelper::uiFont(Utils::StyleHelper::UiElementH4));
    layout->addWidget(title);

    auto *intro = new QLabel(
        Tr::tr("Using the Agentic AI Chat requires you to accept the following terms and "
               "conditions:"),
        this);
    intro->setWordWrap(true);
    layout->addWidget(intro);

    auto *terms = new QTextBrowser(this);
    terms->setOpenExternalLinks(true);
    terms->setMarkdown(termsText());
    layout->addWidget(terms, 1);

    const QString confirmationText
        = Tr::tr("I confirm that I have reviewed and accept the terms and conditions of this "
                 "extension. I confirm that I have the authority and ability to accept the "
                 "terms and conditions of this extension for the customer. I acknowledge that "
                 "if the customer and the Qt Company already have a valid agreement in place, "
                 "that agreement shall apply, but these terms shall govern the use of this "
                 "extension.");

    auto *confirmationBox = new QCheckBox(this);
    confirmationBox->setObjectName("acpConfirmTermsCheckBox");
    confirmationBox->setAccessibleName(Tr::tr("Accept the terms and conditions"));
    confirmationBox->setAccessibleDescription(confirmationText);

    auto *confirmationLabel = new ConfirmationLabel(confirmationText, confirmationBox, this);
    confirmationLabel->setObjectName("acpConfirmTermsLabel");
    confirmationLabel->setBuddy(confirmationBox);

    auto *confirmationRow = new QHBoxLayout;
    confirmationRow->setSpacing(GapHM);
    confirmationRow->addWidget(confirmationBox, 0, Qt::AlignVCenter);
    confirmationRow->addWidget(confirmationLabel, 1);
    layout->addLayout(confirmationRow);

    auto *declineButton = new Utils::QtcButton(Tr::tr("Decline"),
                                               Utils::QtcButton::MediumSecondary, this);
    auto *acceptButton = new Utils::QtcButton(Tr::tr("Accept"),
                                              Utils::QtcButton::MediumPrimary, this);
    acceptButton->setEnabled(false);

    auto *buttonRow = new QHBoxLayout;
    buttonRow->setSpacing(GapHS);
    buttonRow->addStretch();
    buttonRow->addWidget(declineButton);
    buttonRow->addWidget(acceptButton);
    layout->addLayout(buttonRow);

    connect(confirmationBox, &QCheckBox::toggled, acceptButton, &QWidget::setEnabled);
    connect(acceptButton, &QAbstractButton::clicked, this, [this] {
        setAcpTermsAccepted(true);
        emit accepted();
    });
    connect(declineButton, &QAbstractButton::clicked, this, &AcpTermsWidget::declined);
}

} // namespace AcpClient::Internal
