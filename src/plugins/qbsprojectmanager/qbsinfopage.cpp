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

#include "qbsinfopage.h"
#include "ui_qbsinfowidget.h"

#include "qbsprojectmanagerconstants.h"

#include <qbs.h>

namespace QbsProjectManager {
namespace Internal {

class QbsInfoWidget : public QWidget
{
    Q_OBJECT
public:
    QbsInfoWidget(QWidget *parent = 0) : QWidget(parent)
    {
        m_ui.setupUi(this);
        m_ui.versionValueLabel->setText(qbs::LanguageInfo::qbsVersion());
    }

    Ui::QbsInfoWidget m_ui;
};


QbsInfoPage::QbsInfoPage(QObject *parent) : Core::IOptionsPage(parent), m_widget(nullptr)
{
    setId("AB.QbsInfo");
    setDisplayName(QCoreApplication::translate("QbsProjectManager", "Version Info"));
    setCategory(Constants::QBS_SETTINGS_CATEGORY);
    setDisplayCategory(QCoreApplication::translate("QbsProjectManager",
                                                   Constants::QBS_SETTINGS_TR_CATEGORY));
    setCategoryIcon(Utils::Icon(Constants::QBS_SETTINGS_CATEGORY_ICON));
}

QWidget *QbsInfoPage::widget()
{
    if (!m_widget)
        m_widget = new QbsInfoWidget;
    return m_widget;
}

void QbsInfoPage::finish()
{
    delete m_widget;
    m_widget = nullptr;
}

} // namespace Internal
} // namespace QbsProjectManager

#include "qbsinfopage.moc"
