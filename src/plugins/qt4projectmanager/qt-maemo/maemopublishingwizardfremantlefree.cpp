/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
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
#include "maemopublishingwizardfremantlefree.h"

#include "maemopublishingresultpagefremantlefree.h"
#include "maemopublisherfremantlefree.h"
#include "maemopublishinguploadsettingspagefremantlefree.h"
#include "maemopublishingbuildsettingspagefremantlefree.h"

using namespace ProjectExplorer;

namespace Qt4ProjectManager {
namespace Internal {
namespace {
enum PageId { BuildSettingsPageId, UploadSettingsPageId, ResultPageId };
} // anonymous namespace

MaemoPublishingWizardFremantleFree::MaemoPublishingWizardFremantleFree(const Project *project,
    QWidget *parent) :
    QWizard(parent),
    m_project(project),
    m_publisher(new MaemoPublisherFremantleFree(project, this))
{
    setOption(NoCancelButton, false);
    const QString titleText
        = tr("Publishing to Fremantle's \"Extras-devel free\" Repository");
    m_buildSettingsPage
        = new MaemoPublishingBuildSettingsPageFremantleFree(project, m_publisher);
    m_buildSettingsPage->setTitle(titleText);
    m_buildSettingsPage->setSubTitle(tr("Build Settings"));
    setPage(BuildSettingsPageId, m_buildSettingsPage);
    m_uploadSettingsPage
        = new MaemoPublishingUploadSettingsPageFremantleFree(m_publisher);
    m_uploadSettingsPage->setTitle(titleText);
    m_uploadSettingsPage->setSubTitle(tr("Upload Settings"));
    m_uploadSettingsPage->setCommitPage(true);
    setPage(UploadSettingsPageId, m_uploadSettingsPage);
    m_resultPage = new MaemoPublishingResultPageFremantleFree(m_publisher);
    m_resultPage->setTitle(titleText);
    m_resultPage->setSubTitle(tr("Result"));
    setPage(ResultPageId, m_resultPage);
}

int MaemoPublishingWizardFremantleFree::nextId() const
{
    if (currentPage()->isCommitPage())
        return ResultPageId;
    return QWizard::nextId();
}


} // namespace Internal
} // namespace Qt4ProjectManager
