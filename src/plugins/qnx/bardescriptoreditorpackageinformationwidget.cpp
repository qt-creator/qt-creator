/**************************************************************************
**
** Copyright (C) 2011 - 2013 Research In Motion
**
** Contact: Research In Motion (blackberry-qt@qnx.com)
** Contact: KDAB (info@kdab.com)
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

#include "bardescriptoreditorpackageinformationwidget.h"
#include "ui_bardescriptoreditorpackageinformationwidget.h"

using namespace Qnx;
using namespace Qnx::Internal;

BarDescriptorEditorPackageInformationWidget::BarDescriptorEditorPackageInformationWidget(QWidget *parent) :
    BarDescriptorEditorAbstractPanelWidget(parent),
    m_ui(new Ui::BarDescriptorEditorPackageInformationWidget)
{
    m_ui->setupUi(this);

    QRegExp versionNumberRegExp(QLatin1String("(\\d{1,3}\\.)?(\\d{1,3}\\.)?(\\d{1,3})"));
    QRegExpValidator *versionNumberValidator = new QRegExpValidator(versionNumberRegExp, this);
    m_ui->packageVersion->setValidator(versionNumberValidator);

    connect(m_ui->packageId, SIGNAL(textChanged(QString)), this, SIGNAL(changed()));
    connect(m_ui->packageVersion, SIGNAL(textChanged(QString)), this, SIGNAL(changed()));
    connect(m_ui->packageBuildId, SIGNAL(textChanged(QString)), this, SIGNAL(changed()));
}

BarDescriptorEditorPackageInformationWidget::~BarDescriptorEditorPackageInformationWidget()
{
    delete m_ui;
}

void BarDescriptorEditorPackageInformationWidget::clear()
{
    setLineEditBlocked(m_ui->packageId, QString());
    setLineEditBlocked(m_ui->packageVersion, QString());
    setLineEditBlocked(m_ui->packageBuildId, QString());
}

QString BarDescriptorEditorPackageInformationWidget::packageId() const
{
    return m_ui->packageId->text();
}

void BarDescriptorEditorPackageInformationWidget::setPackageId(const QString &packageId)
{
    setLineEditBlocked(m_ui->packageId, packageId);
}

QString BarDescriptorEditorPackageInformationWidget::packageVersion() const
{
    QString version = m_ui->packageVersion->text();
    int pos = 0;
    if (m_ui->packageVersion->validator()->validate(version, pos) == QValidator::Intermediate) {
        if (version.endsWith(QLatin1Char('.')))
            version = version.left(version.size() - 1);
    }
    return version;
}

void BarDescriptorEditorPackageInformationWidget::setPackageVersion(const QString &packageVersion)
{
    setLineEditBlocked(m_ui->packageVersion, packageVersion);
}

QString BarDescriptorEditorPackageInformationWidget::packageBuildId() const
{
    return m_ui->packageBuildId->text();
}

void BarDescriptorEditorPackageInformationWidget::setPackageBuildId(const QString &packageBuildId)
{
    setLineEditBlocked(m_ui->packageBuildId, packageBuildId);
}
