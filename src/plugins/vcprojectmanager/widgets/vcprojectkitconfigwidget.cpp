/**************************************************************************
**
** Copyright (c) 2014 Bojan Petrovic
** Copyright (c) 2014 Radovan Zivkovic
** Contact: http://www.qt-project.org/legal
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
#include "vcprojectkitconfigwidget.h"
#include "../msbuildversionmanager.h"
#include "../vcprojectkitinformation.h"

#include <QComboBox>

namespace VcProjectManager {
namespace Internal {

VcProjectKitConfigWidget::VcProjectKitConfigWidget(ProjectExplorer::Kit *k,
                                                   const ProjectExplorer::KitInformation *ki)
    : ProjectExplorer::KitConfigWidget(k, ki)
{
    m_comboBox = new QComboBox;
    MsBuildVersionManager *msBVM = MsBuildVersionManager::instance();

    foreach (MsBuildInformation *msBuildInfo, msBVM->msBuildInformations())
        insertMsBuild(msBuildInfo);

    refresh();

    connect(m_comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(currentMsBuildChanged(int)));
    connect(msBVM, SIGNAL(msBuildAdded(Core::Id)), this, SLOT(onMsBuildAdded(Core::Id)));
    connect(msBVM, SIGNAL(msBuildReplaced(Core::Id,Core::Id)), this, SLOT(onMsBuildReplaced(Core::Id,Core::Id)));
    connect(msBVM, SIGNAL(msBuildRemoved(Core::Id)), this, SLOT(onMsBuildRemoved(Core::Id)));
}

VcProjectKitConfigWidget::~VcProjectKitConfigWidget()
{
    delete m_comboBox;
}

QString VcProjectKitConfigWidget::displayName() const
{
    return tr("Ms Build version:");
}

void VcProjectKitConfigWidget::makeReadOnly()
{
    m_comboBox->setEnabled(false);
}

void VcProjectKitConfigWidget::refresh()
{
    m_comboBox->setCurrentIndex(indexOf(VcProjectKitInformation::msBuildInfo(m_kit)));
}

bool VcProjectKitConfigWidget::visibleInKit()
{
    return true;
}

QWidget *VcProjectKitConfigWidget::mainWidget() const
{
    return m_comboBox;
}

QWidget *VcProjectKitConfigWidget::buttonWidget() const
{
    return 0;
}

void VcProjectKitConfigWidget::currentMsBuildChanged(int index)
{
    if (0 <= index && index < m_comboBox->count()) {
        Core::Id id = Core::Id::fromSetting(m_comboBox->itemData(index));
        MsBuildInformation *msBuild = MsBuildVersionManager::instance()->msBuildInformation(id);
        VcProjectKitInformation::setMsBuild(m_kit, msBuild);
    }
}

void VcProjectKitConfigWidget::onMsBuildAdded(Core::Id msBuildId)
{
    insertMsBuild(MsBuildVersionManager::instance()->msBuildInformation(msBuildId));
}

void VcProjectKitConfigWidget::onMsBuildReplaced(Core::Id oldMsBuildId, Core::Id newMsBuildId)
{
    int index = indexOf(MsBuildVersionManager::instance()->msBuildInformation(oldMsBuildId));

    if (index < 0 || index >= m_comboBox->count())
        return;

    MsBuildInformation *msBuild = MsBuildVersionManager::instance()->msBuildInformation(newMsBuildId);
    m_comboBox->setItemText(index, msBuild->m_versionString);
    m_comboBox->setItemData(index, msBuild->getId().toSetting());
}

void VcProjectKitConfigWidget::onMsBuildRemoved(Core::Id msBuildId)
{
    removeMsBuild(MsBuildVersionManager::instance()->msBuildInformation(msBuildId));
}

int VcProjectKitConfigWidget::indexOf(MsBuildInformation *msBuild) const
{
    if (!msBuild)
        return -1;

    for (int i = 0; i < m_comboBox->count(); ++i)
        if (msBuild->getId() == Core::Id::fromSetting(m_comboBox->itemData(i)))
            return i;

    return -1;
}

void VcProjectKitConfigWidget::insertMsBuild(MsBuildInformation *msBuild)
{
    if (msBuild)
        m_comboBox->addItem(msBuild->m_versionString, msBuild->getId().toSetting());

    if (!m_comboBox->isEnabled())
        m_comboBox->setEnabled(true);
}

void VcProjectKitConfigWidget::removeMsBuild(MsBuildInformation *msBuild)
{
    if (msBuild) {
        m_comboBox->removeItem(indexOf(msBuild));

        if (m_comboBox->count() == 0) {
            m_comboBox->addItem(tr("<No Ms Build tools available>"));
            m_comboBox->setEnabled(false);
        }
    }
}

} // namespace Internal
} // namespace VcProjectManager
