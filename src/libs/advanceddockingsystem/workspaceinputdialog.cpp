// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-2.1-or-later OR GPL-3.0-or-later

#include "workspaceinputdialog.h"

#include "advanceddockingsystemtr.h"
#include "dockmanager.h"

#include <utils/algorithm.h>
#include <utils/layoutbuilder.h>

#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QValidator>

namespace ADS {

class WorkspaceValidator : public QValidator
{
public:
    WorkspaceValidator(QObject *parent = nullptr);
    QValidator::State validate(QString &input, int &pos) const override;
};

WorkspaceValidator::WorkspaceValidator(QObject *parent)
    : QValidator(parent)
{}

QValidator::State WorkspaceValidator::validate(QString &input, int &pos) const
{
    Q_UNUSED(pos)

    static const QRegularExpression rx("^[a-zA-Z0-9 ()]*$");

    if (!rx.match(input).hasMatch())
        return QValidator::Invalid;

    if (input.isEmpty())
        return QValidator::Intermediate;

    return QValidator::Acceptable;
}

WorkspaceNameInputDialog::WorkspaceNameInputDialog(DockManager *manager, QWidget *parent)
    : QDialog(parent)
    , m_manager(manager)
{
    auto label = new QLabel(Tr::tr("Enter the name of the workspace:"), this);
    m_newWorkspaceLineEdit = new QLineEdit(this);
    m_newWorkspaceLineEdit->setValidator(new WorkspaceValidator(this));
    auto buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                                        Qt::Horizontal,
                                        this);
    m_okButton = buttons->button(QDialogButtonBox::Ok);
    m_switchToButton = new QPushButton;
    buttons->addButton(m_switchToButton, QDialogButtonBox::AcceptRole);
    connect(m_switchToButton, &QPushButton::clicked, this, [this] { m_usedSwitchTo = true; });
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    m_okButton->setEnabled(false);
    m_switchToButton->setEnabled(false);

    connect(m_newWorkspaceLineEdit, &QLineEdit::textChanged, this, [this](const QString &) {
        m_okButton->setEnabled(m_newWorkspaceLineEdit->hasAcceptableInput());
        m_switchToButton->setEnabled(m_newWorkspaceLineEdit->hasAcceptableInput());
    });

    using namespace Layouting;

    Column {
        label,
        m_newWorkspaceLineEdit,
        buttons
    }.attachTo(this);
}

void WorkspaceNameInputDialog::setActionText(const QString &actionText,
                                             const QString &openActionText)
{
    m_okButton->setText(actionText);
    m_switchToButton->setText(openActionText);
}

void WorkspaceNameInputDialog::setValue(const QString &value)
{
    m_newWorkspaceLineEdit->setText(value);
}

QString WorkspaceNameInputDialog::value() const
{
    return m_newWorkspaceLineEdit->text();
}

bool WorkspaceNameInputDialog::isSwitchToRequested() const
{
    return m_usedSwitchTo;
}

} // namespace ADS
