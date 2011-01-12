/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "passphraseforkeydialog.h"

#include <QtGui/QDialogButtonBox>
#include <QtGui/QLabel>
#include <QtGui/QFormLayout>
#include <QtGui/QLineEdit>
#include <QtGui/QCheckBox>

using namespace Qt4ProjectManager;

PassphraseForKeyDialog::PassphraseForKeyDialog(const QString &keyName, QWidget *parent)
    : QDialog(parent)
{
    QFormLayout *formLayout = new QFormLayout(this);
    setLayout(formLayout);

    QLabel *passphraseLabel = new QLabel(this);
    passphraseLabel->setText(tr("Passphrase:"));
    passphraseLabel->setObjectName(QString::fromUtf8("passphraseLabel"));

    formLayout->setWidget(0, QFormLayout::LabelRole, passphraseLabel);

    QLineEdit *passphraseLineEdit = new QLineEdit(this);
    passphraseLineEdit->setObjectName(QString::fromUtf8("passphraseLineEdit"));
    passphraseLineEdit->setEchoMode(QLineEdit::Password);

    connect(passphraseLineEdit, SIGNAL(textChanged(QString)), this, SLOT(setPassphrase(QString)));

    formLayout->setWidget(0, QFormLayout::FieldRole, passphraseLineEdit);

    m_checkBox = new QCheckBox(this);
    m_checkBox->setText(tr("Save passphrase"));
    m_checkBox->setObjectName(QString::fromUtf8("checkBox"));
    m_checkBox->setToolTip(tr("This is an insecure option. The password will be saved as a plain text."));

    formLayout->setWidget(1, QFormLayout::LabelRole, m_checkBox);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(this);
    buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
    buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    formLayout->setWidget(2, QFormLayout::FieldRole, buttonBox);
    setWindowTitle(tr("Passphrase for %1").arg(keyName));
    setFixedSize( sizeHint() );
}

void PassphraseForKeyDialog::accept()
{
    done(1);
}

void PassphraseForKeyDialog::reject()
{
    done(0);
}

void PassphraseForKeyDialog::setPassphrase(const QString &passphrase)
{
    m_passphrase = passphrase;
}

QString PassphraseForKeyDialog::passphrase() const
{
    return m_passphrase;
}

bool PassphraseForKeyDialog::savePassphrase() const
{
    return m_checkBox->isChecked();
}
