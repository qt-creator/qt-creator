// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "startremotedialog.h"

#include "../debuggertr.h"

#include <coreplugin/icore.h>

#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/devicesupport/sshparameters.h>
#include <projectexplorer/kitaspects.h>
#include <projectexplorer/kitchooser.h>

#include <utils/commandline.h>

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QPushButton>

using namespace ProjectExplorer;
using namespace Utils;

namespace Debugger {
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
    setWindowTitle(Tr::tr("Start Remote Analysis"));

    d->kitChooser = new KitChooser(this);
    d->kitChooser->setKitPredicate([](const Kit *kit) {
        const IDevice::ConstPtr device = DeviceKitAspect::device(kit);
        return kit->isValid() && device && !device->sshParameters().host().isEmpty();
    });
    d->executable = new QLineEdit(this);
    d->arguments = new QLineEdit(this);
    d->workingDirectory = new QLineEdit(this);

    d->buttonBox = new QDialogButtonBox(this);
    d->buttonBox->setOrientation(Qt::Horizontal);
    d->buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

    auto formLayout = new QFormLayout;
    formLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    formLayout->addRow(Tr::tr("Kit:"), d->kitChooser);
    formLayout->addRow(Tr::tr("Executable:"), d->executable);
    formLayout->addRow(Tr::tr("Arguments:"), d->arguments);
    formLayout->addRow(Tr::tr("Working directory:"), d->workingDirectory);

    auto verticalLayout = new QVBoxLayout(this);
    verticalLayout->addLayout(formLayout);
    verticalLayout->addWidget(d->buttonBox);

    QtcSettings *settings = Core::ICore::settings();
    settings->beginGroup("AnalyzerStartRemoteDialog");
    d->kitChooser->populate();
    d->kitChooser->setCurrentKitId(Id::fromSetting(settings->value("profile")));
    d->executable->setText(settings->value("executable").toString());
    d->workingDirectory->setText(settings->value("workingDirectory").toString());
    d->arguments->setText(settings->value("arguments").toString());
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
    QtcSettings *settings = Core::ICore::settings();
    settings->beginGroup("AnalyzerStartRemoteDialog");
    settings->setValue("profile", d->kitChooser->currentKitId().toString());
    settings->setValue("executable", d->executable->text());
    settings->setValue("workingDirectory", d->workingDirectory->text());
    settings->setValue("arguments", d->arguments->text());
    settings->endGroup();

    QDialog::accept();
}

void StartRemoteDialog::validate()
{
    bool valid = !d->executable->text().isEmpty();
    d->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(valid);
}

CommandLine StartRemoteDialog::commandLine() const
{
    const Kit *kit = d->kitChooser->currentKit();
    const FilePath filePath = DeviceKitAspect::deviceFilePath(kit, d->executable->text());
    return {filePath, d->arguments->text(), CommandLine::Raw};
}

FilePath StartRemoteDialog::workingDirectory() const
{
    return FilePath::fromString(d->workingDirectory->text());
}

} // Debugger
