/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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

#ifndef MAEMOPUBLISHINGWIZARDFREMANTLEFREE_H
#define MAEMOPUBLISHINGWIZARDFREMANTLEFREE_H

#include <utils/wizard.h>

namespace ProjectExplorer {
class Project;
}

namespace Madde {
namespace Internal {
class MaemoPublishingResultPageFremantleFree;
class MaemoPublisherFremantleFree;
class MaemoPublishingUploadSettingsPageFremantleFree;
class MaemoPublishingBuildSettingsPageFremantleFree;

class MaemoPublishingWizardFremantleFree : public Utils::Wizard
{
    Q_OBJECT
public:
    explicit MaemoPublishingWizardFremantleFree(const ProjectExplorer::Project *project,
                                                QWidget *parent = 0);

private:
    virtual int nextId() const;

    const ProjectExplorer::Project * const m_project;
    MaemoPublisherFremantleFree * const m_publisher;
    MaemoPublishingBuildSettingsPageFremantleFree *m_buildSettingsPage;
    MaemoPublishingUploadSettingsPageFremantleFree *m_uploadSettingsPage;
    MaemoPublishingResultPageFremantleFree *m_resultPage;
};

} // namespace Internal
} // namespace Madde

#endif // MAEMOPUBLISHINGWIZARDFREMANTLEFREE_H
