/****************************************************************************
**
** Copyright (C) 2016 Tim Sander <tim@krieglstein.org>
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "baremetalrunconfigurationwidget.h"
#include "baremetalrunconfiguration.h"

#include <projectexplorer/runconfigurationaspects.h>
#include <utils/detailswidget.h>
#include <utils/utilsicons.h>

#include <QLineEdit>
#include <QFormLayout>
#include <QLabel>
#include <QCoreApplication>
#include <QDir>

using namespace ProjectExplorer;

namespace BareMetal {
namespace Internal {

class BareMetalRunConfigurationWidgetPrivate
{
public:
    BareMetalRunConfigurationWidgetPrivate(BareMetalRunConfiguration *runConfig)
        : runConfiguration(runConfig)
    { }

    BareMetalRunConfiguration * const runConfiguration;
    QLineEdit workingDirLineEdit;
    QLabel localExecutableLabel;
    QFormLayout genericWidgetsLayout;
};

} // namespace Internal

using namespace Internal;

BareMetalRunConfigurationWidget::BareMetalRunConfigurationWidget(BareMetalRunConfiguration *runConfiguration,
                                                                 QWidget *parent)
    : QWidget(parent), d(new BareMetalRunConfigurationWidgetPrivate(runConfiguration))
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setMargin(0);

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
    runConfiguration->extraAspect<ArgumentsAspect>()->addToMainConfigurationWidget(this, &d->genericWidgetsLayout);

    d->workingDirLineEdit.setPlaceholderText(tr("<default>"));
    d->workingDirLineEdit.setText(d->runConfiguration->workingDirectory());
    d->genericWidgetsLayout.addRow(tr("Working directory:"), &d->workingDirLineEdit);
    connect(d->runConfiguration, &BareMetalRunConfiguration::targetInformationChanged,
            this, &BareMetalRunConfigurationWidget::updateTargetInformation);
    connect(&d->workingDirLineEdit, &QLineEdit::textEdited,
            this, &BareMetalRunConfigurationWidget::handleWorkingDirectoryChanged);
}

BareMetalRunConfigurationWidget::~BareMetalRunConfigurationWidget()
{
    delete d;
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

} // namespace BareMetal
