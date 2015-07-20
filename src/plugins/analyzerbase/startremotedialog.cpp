/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "startremotedialog.h"

#include <coreplugin/icore.h>
#include <projectexplorer/kitchooser.h>
#include <projectexplorer/kitinformation.h>
#include <ssh/sshconnection.h>

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QPushButton>

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
    d->kitChooser->setKitMatcher([](const Kit *kit) {
        const IDevice::ConstPtr device = DeviceKitInformation::device(kit);
        return kit->isValid() && device && !device->sshParameters().host.isEmpty();
    });
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
    d->kitChooser->populate();
    d->kitChooser->setCurrentKitId(Core::Id::fromSetting(settings->value(QLatin1String("profile"))));
    d->executable->setText(settings->value(QLatin1String("executable")).toString());
    d->workingDirectory->setText(settings->value(QLatin1String("workingDirectory")).toString());
    d->arguments->setText(settings->value(QLatin1String("arguments")).toString());
    settings->endGroup();

    connect(d->kitChooser, &KitChooser::activated, this, &StartRemoteDialog::validate);
    connect(d->executable, &QLineEdit::textChanged, this, &StartRemoteDialog::validate);
    connect(d->workingDirectory, &QLineEdit::textChanged, this, &StartRemoteDialog::validate);
    connect(d->arguments, &QLineEdit::textChanged, this, &StartRemoteDialog::validate);
    connect(d->buttonBox, &QDialogButtonBox::accepted, this, &StartRemoteDialog::accept);
    connect(d->buttonBox, &QDialogButtonBox::rejected, this, &StartRemoteDialog::reject);

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
