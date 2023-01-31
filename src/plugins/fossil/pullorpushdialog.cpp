// Copyright (c) 2018 Artur Shepilko
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "pullorpushdialog.h"
#include "ui_pullorpushdialog.h"

#include "constants.h"

#include <utils/qtcassert.h>

namespace Fossil {
namespace Internal {

PullOrPushDialog::PullOrPushDialog(Mode mode, QWidget *parent) : QDialog(parent),
    m_mode(mode),
    m_ui(new Ui::PullOrPushDialog)
{
    m_ui->setupUi(this);
    m_ui->localPathChooser->setExpectedKind(Utils::PathChooser::File);
    m_ui->localPathChooser->setPromptDialogFilter(tr(Constants::FOSSIL_FILE_FILTER));

    switch (m_mode) {
    case PullMode:
        this->setWindowTitle(tr("Pull Source"));
        break;
    case PushMode:
        this->setWindowTitle(tr("Push Destination"));
        break;
    }

    // select URL text in line edit when clicking the radio button
    m_ui->localButton->setFocusProxy(m_ui->localPathChooser);
    m_ui->urlButton->setFocusProxy(m_ui->urlLineEdit);
    connect(m_ui->urlButton, &QRadioButton::clicked, m_ui->urlLineEdit, &QLineEdit::selectAll);

    this->adjustSize();
}

PullOrPushDialog::~PullOrPushDialog()
{
    delete m_ui;
}

QString PullOrPushDialog::remoteLocation() const
{
    if (m_ui->defaultButton->isChecked())
        return QString();
    if (m_ui->localButton->isChecked())
        return m_ui->localPathChooser->filePath().toString();
    return m_ui->urlLineEdit->text();
}

bool PullOrPushDialog::isRememberOptionEnabled() const
{
    if (m_ui->defaultButton->isChecked())
        return false;
    return m_ui->rememberCheckBox->isChecked();
}

bool PullOrPushDialog::isPrivateOptionEnabled() const
{
    return m_ui->privateCheckBox->isChecked();
}

void PullOrPushDialog::setDefaultRemoteLocation(const QString &url)
{
    m_ui->urlLineEdit->setText(url);
}

void PullOrPushDialog::setLocalBaseDirectory(const QString &dir)
{
    m_ui->localPathChooser->setBaseDirectory(Utils::FilePath::fromString(dir));
}

void PullOrPushDialog::changeEvent(QEvent *e)
{
    QDialog::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        m_ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

} // namespace Internal
} // namespace Fossil
