/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "qtkitconfigwidget.h"

#include "qtsupportconstants.h"
#include "qtkitinformation.h"
#include "qtversionmanager.h"

#include <coreplugin/icore.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <utils/qtcassert.h>
#include <utils/algorithm.h>

#include <QComboBox>
#include <QPushButton>

namespace QtSupport {
namespace Internal {

QtKitConfigWidget::QtKitConfigWidget(ProjectExplorer::Kit *k, const ProjectExplorer::KitInformation *ki) :
    KitConfigWidget(k, ki)
{
    m_combo = new QComboBox;
    m_combo->addItem(tr("None"), -1);

    QList<int> versionIds = Utils::transform(QtVersionManager::versions(), &BaseQtVersion::uniqueId);
    versionsChanged(versionIds, QList<int>(), QList<int>());

    m_manageButton = new QPushButton(KitConfigWidget::msgManage());

    refresh();
    m_combo->setToolTip(toolTip());

    connect(m_combo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &QtKitConfigWidget::currentWasChanged);

    connect(QtVersionManager::instance(), &QtVersionManager::qtVersionsChanged,
            this, &QtKitConfigWidget::versionsChanged);

    connect(m_manageButton, &QAbstractButton::clicked, this, &QtKitConfigWidget::manageQtVersions);
}

QtKitConfigWidget::~QtKitConfigWidget()
{
    delete m_combo;
    delete m_manageButton;
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

static QString itemNameFor(const BaseQtVersion *v)
{
    QTC_ASSERT(v, return QString());
    QString name = v->displayName();
    if (!v->isValid())
        name = QCoreApplication::translate("QtSupport::Internal::QtKitConfigWidget", "%1 (invalid)").arg(v->displayName());
    return name;
}

void QtKitConfigWidget::versionsChanged(const QList<int> &added, const QList<int> &removed,
                                        const QList<int> &changed)
{
    foreach (const int id, added) {
        BaseQtVersion *v = QtVersionManager::version(id);
        QTC_CHECK(v);
        QTC_CHECK(findQtVersion(id) < 0);
        m_combo->addItem(itemNameFor(v), id);
    }
    foreach (const int id, removed) {
        int pos = findQtVersion(id);
        if (pos >= 0) // We do not include invalid Qt versions, so do not try to remove those.
            m_combo->removeItem(pos);
    }
    foreach (const int id, changed) {
        BaseQtVersion *v = QtVersionManager::version(id);
        int pos = findQtVersion(id);
        QTC_CHECK(pos >= 0);
        m_combo->setItemText(pos, itemNameFor(v));
    }
}

void QtKitConfigWidget::manageQtVersions()
{
    Core::ICore::showOptionsDialog(Constants::QTVERSION_SETTINGS_PAGE_ID, buttonWidget());
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
