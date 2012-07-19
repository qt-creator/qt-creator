/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/

#include "qtprofileconfigwidget.h"

#include "qtsupportconstants.h"
#include "qtprofileinformation.h"
#include "qtversionmanager.h"

#include <coreplugin/icore.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <utils/qtcassert.h>

#include <QComboBox>
#include <QHBoxLayout>
#include <QPushButton>

namespace QtSupport {
namespace Internal {

QtProfileConfigWidget::QtProfileConfigWidget(ProjectExplorer::Profile *p,
                                             QWidget *parent) :
    ProjectExplorer::ProfileConfigWidget(parent),
    m_profile(p),
    m_combo(new QComboBox),
    m_manageButton(new QPushButton(this))
{
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setMargin(0);

    m_combo->setContentsMargins(0, 0, 0, 0);
    m_combo->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
    layout->addWidget(m_combo);

    m_manageButton->setContentsMargins(0, 0, 0, 0);
    m_manageButton->setText(tr("Manage..."));

    QtVersionManager *mgr = QtVersionManager::instance();

    // initially populate combobox:
    m_combo->addItem(tr("None"), -1);
    QList<BaseQtVersion *> versions = mgr->validVersions();
    QList<int> versionIds;
    foreach (BaseQtVersion *v, versions)
        versionIds.append(v->uniqueId());
    versionsChanged(versionIds, QList<int>(), QList<int>());

    discard();
    connect(m_combo, SIGNAL(currentIndexChanged(int)), this, SIGNAL(dirty()));

    connect(mgr, SIGNAL(qtVersionsChanged(QList<int>,QList<int>,QList<int>)),
            this, SLOT(versionsChanged(QList<int>,QList<int>,QList<int>)));
    connect(ProjectExplorer::ProfileManager::instance(), SIGNAL(profileUpdated(ProjectExplorer::Profile*)),
            this, SLOT(profileUpdated(ProjectExplorer::Profile*)));

    connect(m_manageButton, SIGNAL(clicked()), this, SLOT(manageQtVersions()));
}

QString QtProfileConfigWidget::displayName() const
{
    return tr("Qt version:");
}

void QtProfileConfigWidget::makeReadOnly()
{
    m_combo->setEnabled(false);
}

void QtProfileConfigWidget::apply()
{
    int id = m_combo->itemData(m_combo->currentIndex()).toInt();
    QtProfileInformation::setQtVersionId(m_profile, id);
}

void QtProfileConfigWidget::discard()
{
    m_combo->setCurrentIndex(findQtVersion(QtProfileInformation::qtVersionId(m_profile)));
}

bool QtProfileConfigWidget::isDirty() const
{
    int id = m_combo->itemData(m_combo->currentIndex()).toInt();
    return id != QtProfileInformation::qtVersionId(m_profile);
}

QWidget *QtProfileConfigWidget::buttonWidget() const
{
    return m_manageButton;
}

void QtProfileConfigWidget::versionsChanged(const QList<int> &added, const QList<int> &removed,
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

void QtProfileConfigWidget::profileUpdated(ProjectExplorer::Profile *p)
{
    if (p != m_profile)
        return;

    int id = QtProfileInformation::qtVersionId(p);

    for (int i = 0; i < m_combo->count(); ++i) {
        if (m_combo->itemData(i).toInt() == id) {
            m_combo->setCurrentIndex(i);
            break;
        }
    }
}

void QtProfileConfigWidget::manageQtVersions()
{
    Core::ICore::showOptionsDialog(QLatin1String(ProjectExplorer::Constants::PROJECTEXPLORER_SETTINGS_CATEGORY),
                                   QLatin1String(QtSupport::Constants::QTVERSION_SETTINGS_PAGE_ID));
}

int QtProfileConfigWidget::findQtVersion(const int id) const
{
    for (int i = 0; i < m_combo->count(); ++i) {
        if (id == m_combo->itemData(i).toInt())
            return i;
    }
    return -1;
}

} // namespace Internal
} // namespace QtSupport
