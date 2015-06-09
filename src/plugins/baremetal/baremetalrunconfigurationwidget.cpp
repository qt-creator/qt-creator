/****************************************************************************
**
** Copyright (C) 2015 Tim Sander <tim@krieglstein.org>
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

#include "baremetalrunconfigurationwidget.h"
#include "baremetalrunconfiguration.h"

#include <coreplugin/coreconstants.h>
#include <utils/detailswidget.h>

#include <QLineEdit>
#include <QFormLayout>
#include <QLabel>
#include <QCoreApplication>
#include <QDir>

namespace BareMetal {
namespace Internal {

class BareMetalRunConfigurationWidgetPrivate
{
public:
    BareMetalRunConfigurationWidgetPrivate(BareMetalRunConfiguration *runConfig)
        : runConfiguration(runConfig)
    { }

    BareMetalRunConfiguration * const runConfiguration;
    QWidget topWidget;
    QLabel disabledIcon;
    QLabel disabledReason;
    QLineEdit argsLineEdit;
    QLineEdit workingDirLineEdit;
    QLabel localExecutableLabel;
    QFormLayout genericWidgetsLayout;
};

} // namespace Internal

using namespace Internal;

BareMetalRunConfigurationWidget::BareMetalRunConfigurationWidget(BareMetalRunConfiguration *runConfiguration,
                                                                 QWidget *parent)
    : QWidget(parent),d(new BareMetalRunConfigurationWidgetPrivate(runConfiguration))
{
    QVBoxLayout *topLayout = new QVBoxLayout(this);
    topLayout->setMargin(0);
    addDisabledLabel(topLayout);
    topLayout->addWidget(&d->topWidget);
    QVBoxLayout *mainLayout = new QVBoxLayout(&d->topWidget);
    mainLayout->setMargin(0);
    addGenericWidgets(mainLayout);

    connect(d->runConfiguration, &ProjectExplorer::RunConfiguration::enabledChanged,
            this, &BareMetalRunConfigurationWidget::runConfigurationEnabledChange);
    runConfigurationEnabledChange();
}

BareMetalRunConfigurationWidget::~BareMetalRunConfigurationWidget()
{
    delete d;
}

void BareMetalRunConfigurationWidget::addDisabledLabel(QVBoxLayout *topLayout)
{
    QHBoxLayout * const hl = new QHBoxLayout;
    hl->addStretch();
    d->disabledIcon.setPixmap(QPixmap(QLatin1String(Core::Constants::ICON_WARNING)));
    hl->addWidget(&d->disabledIcon);
    d->disabledReason.setVisible(false);
    hl->addWidget(&d->disabledReason);
    hl->addStretch();
    topLayout->addLayout(hl);
}

void BareMetalRunConfigurationWidget::addGenericWidgets(QVBoxLayout *mainLayout)
{
    Utils::DetailsWidget *detailsContainer = new Utils::DetailsWidget(this);
    detailsContainer->setState(Utils::DetailsWidget::NoSummary);

    QWidget *details = new QWidget(this);
    details->setLayout(&d->genericWidgetsLayout);
    detailsContainer->setWidget(details);

    mainLayout->addWidget(detailsContainer);

    d->genericWidgetsLayout.setFormAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    d->localExecutableLabel.setText(d->runConfiguration->localExecutableFilePath());
    d->genericWidgetsLayout.addRow(tr("Executable:"),&d->localExecutableLabel);

    //d->genericWidgetsLayout.addRow(tr("Debugger host:"),d->runConfiguration);
    //d->genericWidgetsLayout.addRow(tr("Debugger port:"),d->runConfiguration);
    d->argsLineEdit.setText(d->runConfiguration->arguments());
    d->genericWidgetsLayout.addRow(tr("Arguments:"), &d->argsLineEdit);

    d->workingDirLineEdit.setPlaceholderText(tr("<default>"));
    d->workingDirLineEdit.setText(d->runConfiguration->workingDirectory());
    d->genericWidgetsLayout.addRow(tr("Working directory:"), &d->workingDirLineEdit);
    connect(&d->argsLineEdit, &QLineEdit::textEdited,
            this, &BareMetalRunConfigurationWidget::argumentsEdited);
    connect(d->runConfiguration, &BareMetalRunConfiguration::targetInformationChanged,
            this, &BareMetalRunConfigurationWidget::updateTargetInformation);
    connect(&d->workingDirLineEdit, &QLineEdit::textEdited,
            this, &BareMetalRunConfigurationWidget::handleWorkingDirectoryChanged);
}

void BareMetalRunConfigurationWidget::argumentsEdited(const QString &args)
{
    d->runConfiguration->setArguments(args);
}

void BareMetalRunConfigurationWidget::updateTargetInformation()
{
    setLabelText(d->localExecutableLabel,
            QDir::toNativeSeparators(d->runConfiguration->localExecutableFilePath()),
            tr("Unknown"));
}

void BareMetalRunConfigurationWidget::handleWorkingDirectoryChanged()
{
    d->runConfiguration->setWorkingDirectory(d->workingDirLineEdit.text().trimmed());
}

void BareMetalRunConfigurationWidget::setLabelText(QLabel &label, const QString &regularText, const QString &errorText)
{
    const QString errorMessage = QLatin1String("<font color=\"red\">") + errorText
            + QLatin1String("</font>");
    label.setText(regularText.isEmpty() ? errorMessage : regularText);
}

void BareMetalRunConfigurationWidget::runConfigurationEnabledChange()
{
    bool enabled = d->runConfiguration->isEnabled();
    d->topWidget.setEnabled(enabled);
    d->disabledIcon.setVisible(!enabled);
    d->disabledReason.setVisible(!enabled);
    d->disabledReason.setText(d->runConfiguration->disabledReason());
}

} // namespace BareMetal
