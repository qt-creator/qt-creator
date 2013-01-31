/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include "qtkitconfigwidget.h"

#include "qtsupportconstants.h"
#include "qtkitinformation.h"
#include "qtversionmanager.h"

#include <coreplugin/icore.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <utils/qtcassert.h>

#include <QComboBox>
#include <QHBoxLayout>
#include <QPushButton>

namespace QtSupport {
namespace Internal {

QtKitConfigWidget::QtKitConfigWidget(ProjectExplorer::Kit *k) :
    KitConfigWidget(k)
{
    m_combo = new QComboBox;
    m_combo->addItem(tr("None"), -1);

    QtVersionManager *mgr = QtVersionManager::instance();
    QList<BaseQtVersion *> versions = mgr->validVersions();
    QList<int> versionIds;
    foreach (BaseQtVersion *v, versions)
        versionIds.append(v->uniqueId());
    versionsChanged(versionIds, QList<int>(), QList<int>());

    m_manageButton = new QPushButton(tr("Manage..."));

    refresh();
    connect(m_combo, SIGNAL(currentIndexChanged(int)), this, SLOT(currentWasChanged(int)));

    connect(mgr, SIGNAL(qtVersionsChanged(QList<int>,QList<int>,QList<int>)),
            this, SLOT(versionsChanged(QList<int>,QList<int>,QList<int>)));

    connect(m_manageButton, SIGNAL(clicked()), this, SLOT(manageQtVersions()));
}

QString QtKitConfigWidget::displayName() const
{
    return tr("Qt version:");
}

QString QtKitConfigWidget::toolTip() const
{
    return tr("The Qt library to use for all projects using this kit.<br>"
              "A Qt version is required for qmake-based projects "
              "and optional when using other build systems.");
}

void QtKitConfigWidget::makeReadOnly()
{
    m_combo->setEnabled(false);
}

void QtKitConfigWidget::refresh()
{
    m_combo->setCurrentIndex(findQtVersion(QtKitInformation::qtVersionId(m_kit)));
}

QWidget *QtKitConfigWidget::mainWidget() const
{
    return m_combo;
}

QWidget *QtKitConfigWidget::buttonWidget() const
{
    return m_manageButton;
}

void QtKitConfigWidget::versionsChanged(const QList<int> &added, const QList<int> &removed,
                                        const QList<int> &changed)
{
    QtVersionManager *mgr = QtVersionManager::instance();

    foreach (const int id, added) {
        BaseQtVersion *v = mgr->version(id);
        QTC_CHECK(v);
        QTC_CHECK(findQtVersion(id) < 0);
        m_combo->addItem(v->displayName(), id);
    }
    foreach (const int id, removed) {
        int pos = findQtVersion(id);
        QTC_CHECK(pos >= 0);
        m_combo->removeItem(pos);

    }
    foreach (const int id, changed) {
        BaseQtVersion *v = mgr->version(id);
        int pos = findQtVersion(id);
        QTC_CHECK(pos >= 0);
        m_combo->setItemText(pos, v->displayName());
    }
}

void QtKitConfigWidget::manageQtVersions()
{
    Core::ICore::showOptionsDialog(ProjectExplorer::Constants::PROJECTEXPLORER_SETTINGS_CATEGORY,
                                   Constants::QTVERSION_SETTINGS_PAGE_ID);
}

void QtKitConfigWidget::currentWasChanged(int idx)
{
    QtKitInformation::setQtVersionId(m_kit, m_combo->itemData(idx).toInt());
}

int QtKitConfigWidget::findQtVersion(const int id) const
{
    for (int i = 0; i < m_combo->count(); ++i) {
        if (id == m_combo->itemData(i).toInt())
            return i;
    }
    return -1;
}

} // namespace Internal
} // namespace QtSupport
