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

#include "maemopackagecreationstep.h"

#include <QtGui/QWidget>

using ProjectExplorer::BuildConfiguration;
using ProjectExplorer::BuildStepConfigWidget;


namespace Qt4ProjectManager {
namespace Internal {

class MaemoPackageCreationWidget : public ProjectExplorer::BuildStepConfigWidget
{
public:
    MaemoPackageCreationWidget(MaemoPackageCreationStep *step)
        : ProjectExplorer::BuildStepConfigWidget(), m_step(step) {}
    virtual void init() {}
    virtual QString summaryText() const { return tr("Package Creation"); }
    virtual QString displayName() const { return m_step->displayName(); }
private:
    MaemoPackageCreationStep *m_step;
};

MaemoPackageCreationStep::MaemoPackageCreationStep(BuildConfiguration *buildConfig)
    : ProjectExplorer::BuildStep(buildConfig, CreatePackageId)
{
}

MaemoPackageCreationStep::MaemoPackageCreationStep(BuildConfiguration *buildConfig,
    MaemoPackageCreationStep *other)
    : BuildStep(buildConfig, other)
{

}

bool MaemoPackageCreationStep::init()
{
    return true;
}

void MaemoPackageCreationStep::run(QFutureInterface<bool> &fi)
{
    qDebug("%s", Q_FUNC_INFO);
    fi.reportResult(true);
}

BuildStepConfigWidget *MaemoPackageCreationStep::createConfigWidget()
{
    return new MaemoPackageCreationWidget(this);
}

const QLatin1String MaemoPackageCreationStep::CreatePackageId("Qt4ProjectManager.MaemoPackageCreationStep");

} // namespace Internal
} // namespace Qt4ProjectManager
