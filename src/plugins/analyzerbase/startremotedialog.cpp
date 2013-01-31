/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "startremotedialog.h"

#include <coreplugin/icore.h>
#include <coreplugin/id.h>
#include <projectexplorer/kitchooser.h>
#include <projectexplorer/kitinformation.h>
#include <ssh/sshconnection.h>
#include <utils/pathchooser.h>

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>

using namespace ProjectExplorer;
using namespace Utils;

namespace Analyzer {
namespace Internal {

class StartRemoteDialogPrivate
{
public:
    KitChooser *kitChooser;
    QLineEdit *executable;
    QLineEdit *arguments;
    QLineEdit *workingDirectory;
    QDialogButtonBox *buttonBox;
};

} // namespace Internal

StartRemoteDialog::StartRemoteDialog(QWidget *parent)
    : QDialog(parent)
    , d(new Internal::StartRemoteDialogPrivate)
{
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setWindowTitle(tr("Start Remote Analysis"));

    d->kitChooser = new KitChooser(this);
    d->executable = new QLineEdit(this);
    d->arguments = new QLineEdit(this);
    d->workingDirectory = new QLineEdit(this);

    d->buttonBox = new QDialogButtonBox(this);
    d->buttonBox->setOrientation(Qt::Horizontal);
    d->buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

    QFormLayout *formLayout = new QFormLayout;
    formLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    formLayout->addRow(tr("Kit:"), d->kitChooser);
    formLayout->addRow(tr("Executable:"), d->executable);
    formLayout->addRow(tr("Arguments:"), d->arguments);
    formLayout->addRow(tr("Working directory:"), d->workingDirectory);

    QVBoxLayout *verticalLayout = new QVBoxLayout(this);
    verticalLayout->addLayout(formLayout);
    verticalLayout->addWidget(d->buttonBox);

    QSettings *settings = Core::ICore::settings();
    settings->beginGroup(QLatin1String("AnalyzerStartRemoteDialog"));
    QString kit = settings->value(QLatin1String("profile")).toString();
    d->kitChooser->populate();
    d->kitChooser->setCurrentKitId(Core::Id(kit));
    d->executable->setText(settings->value(QLatin1String("executable")).toString());
    d->workingDirectory->setText(settings->value(QLatin1String("workingDirectory")).toString());
    d->arguments->setText(settings->value(QLatin1String("arguments")).toString());
    settings->endGroup();

    connect(d->kitChooser, SIGNAL(activated(int)), SLOT(validate()));
    connect(d->executable, SIGNAL(textChanged(QString)), SLOT(validate()));
    connect(d->workingDirectory, SIGNAL(textChanged(QString)), SLOT(validate()));
    connect(d->arguments, SIGNAL(textChanged(QString)), SLOT(validate()));
    connect(d->buttonBox, SIGNAL(accepted()), SLOT(accept()));
    connect(d->buttonBox, SIGNAL(rejected()), SLOT(reject()));

    validate();
}

StartRemoteDialog::~StartRemoteDialog()
{
    delete d;
}

void StartRemoteDialog::accept()
{
    QSettings *settings = Core::ICore::settings();
    settings->beginGroup(QLatin1String("AnalyzerStartRemoteDialog"));
    settings->setValue(QLatin1String("profile"), d->kitChooser->currentKitId().toString());
    settings->setValue(QLatin1String("executable"), d->executable->text());
    settings->setValue(QLatin1String("workingDirectory"), d->workingDirectory->text());
    settings->setValue(QLatin1String("arguments"), d->arguments->text());
    settings->endGroup();

    QDialog::accept();
}

void StartRemoteDialog::validate()
{
    bool valid = !d->executable->text().isEmpty();
    d->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(valid);
}

QSsh::SshConnectionParameters StartRemoteDialog::sshParams() const
{
    Kit *kit = d->kitChooser->currentKit();
    IDevice::ConstPtr device = DeviceKitInformation::device(kit);
    return device->sshParameters();
}

QString StartRemoteDialog::executable() const
{
    return d->executable->text();
}

QString StartRemoteDialog::arguments() const
{
    return d->arguments->text();
}

QString StartRemoteDialog::workingDirectory() const
{
    return d->workingDirectory->text();
}

} // namespace Analyzer
