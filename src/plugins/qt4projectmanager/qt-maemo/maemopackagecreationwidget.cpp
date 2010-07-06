/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of Qt Creator.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
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
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "maemopackagecreationwidget.h"
#include "ui_maemopackagecreationwidget.h"

#include "maemodeployablelistmodel.h"
#include "maemodeployablelistwidget.h"
#include "maemodeployables.h"
#include "maemopackagecreationstep.h"
#include "maemotoolchain.h"

#include <utils/qtcassert.h>
#include <projectexplorer/project.h>
#include <projectexplorer/target.h>
#include <qt4projectmanager/qt4buildconfiguration.h>

namespace Qt4ProjectManager {
namespace Internal {

MaemoPackageCreationWidget::MaemoPackageCreationWidget(MaemoPackageCreationStep *step)
    : ProjectExplorer::BuildStepConfigWidget(),
      m_step(step),
      m_ui(new Ui::MaemoPackageCreationWidget)
{
    m_ui->setupUi(this);
    m_ui->skipCheckBox->setChecked(!m_step->isPackagingEnabled());
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    const QStringList list = m_step->versionString().split(QLatin1Char('.'),
        QString::SkipEmptyParts);
    m_ui->major->setValue(list.value(0, QLatin1String("0")).toInt());
    m_ui->minor->setValue(list.value(1, QLatin1String("0")).toInt());
    m_ui->patch->setValue(list.value(2, QLatin1String("0")).toInt());
    versionInfoChanged();   // workaround for missing minor and patch update notifications
    for (int i = 0; i < step->deployables()->modelCount(); ++i) {
        MaemoDeployableListModel * const model
            = step->deployables()->modelAt(i);
        m_ui->tabWidget->addTab(new MaemoDeployableListWidget(this, model),
            model->projectName());
    }
}

void MaemoPackageCreationWidget::init()
{
}

QString MaemoPackageCreationWidget::summaryText() const
{
    return tr("<b>Create Package:</b> ") + QDir::toNativeSeparators(m_step->packageFilePath());
}

QString MaemoPackageCreationWidget::displayName() const
{
    return m_step->displayName();
}

void MaemoPackageCreationWidget::handleSkipButtonToggled(bool checked)
{
    m_step->setPackagingEnabled(!checked);
}

void MaemoPackageCreationWidget::versionInfoChanged()
{
    m_step->setVersionString(m_ui->major->text() + QLatin1Char('.')
        + m_ui->minor->text() + QLatin1Char('.') + m_ui->patch->text());
    emit updateSummary();
}

} // namespace Internal
} // namespace Qt4ProjectManager
