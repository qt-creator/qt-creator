/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "s60publishingwizardfactories.h"
#include "s60publishingwizardovi.h"

#include "qmakestep.h"
#include "qt4project.h"
#include "qt4projectmanagerconstants.h"

#include <projectexplorer/target.h>

#include <qtsupport/qtsupportconstants.h>
#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtprofileinformation.h>

using namespace ProjectExplorer;

namespace Qt4ProjectManager {
namespace Internal {
S60PublishingWizardFactoryOvi::S60PublishingWizardFactoryOvi(QObject *parent)
    : IPublishingWizardFactory(parent)
{}

QString S60PublishingWizardFactoryOvi::displayName() const
{
    return tr("Publish Qt Symbian Applications to Nokia Store");
}

QString S60PublishingWizardFactoryOvi::description() const
{
    return tr("This wizard checks "
              "your project file to make sure it complies with "
              "Nokia Store submission criteria.\n\n"
              "The wizard creates SIS files that can be submitted "
              "to Nokia Publish.\n\n"
              "You cannot use it if you use application UIDs from Symbian Signed.\n\n"
              "You cannot use it for the Certified Signed and Manufacturer level capabilities:\n"
              "NetworkControl, MultimediaDD, CommDD, DiskAdmin, AllFiles, DRM and TCB.\n\n"
              "Your application will also be rejected by Nokia Store QA if you choose "
              "an unreleased Qt version on the next page.");
}

bool S60PublishingWizardFactoryOvi::canCreateWizard(const Project *project) const
{
    if (!qobject_cast<const Qt4Project *>(project))
        return false;
    foreach (const Target *const target, project->targets()) {
        QtSupport::BaseQtVersion *version = QtSupport::QtProfileInformation::qtVersion(target->profile());
        if (version->type() == QLatin1String(QtSupport::Constants::SYMBIANQT))
            return true;
    }
    return false;
}

QWizard *S60PublishingWizardFactoryOvi::createWizard(const Project *project) const
{
    Q_ASSERT(canCreateWizard(project));
    return new S60PublishingWizardOvi(project);
}

} // namespace Internal
} // namespace Qt4ProjectManager

