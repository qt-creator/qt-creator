/**************************************************************************
**
** Copyright (C) 2014 BlackBerry Limited. All rights reserved.
**
** Contact: BlackBerry (qt@blackberry.com)
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

#include "qnxsettingspage.h"
#include "qnxconfiguration.h"
#include "qnxconfigurationmanager.h"
#include "qnxconstants.h"

#include <qnxsettingswidget.h>

#include <coreplugin/icore.h>

#include <QCoreApplication>

#include <QMessageBox>

namespace Qnx {
namespace Internal {

QnxSettingsPage::QnxSettingsPage(QObject* parent) :
    Core::IOptionsPage(parent)
{
    setId(Core::Id(Constants::QNX_SETTINGS_ID));
    setDisplayName(tr("QNX"));
    setCategory(Constants::QNX_CATEGORY);
    setDisplayCategory(QCoreApplication::translate("QNX",
                Constants::QNX_CATEGORY_TR));
    setCategoryIcon(QLatin1String(Constants::QNX_CATEGORY_ICON));
}

QWidget* QnxSettingsPage::widget()
{
    if (!m_widget)
        m_widget = new QnxSettingsWidget;
    return m_widget;
}

void QnxSettingsPage::apply()
{
    m_widget->applyChanges();
}

void QnxSettingsPage::finish()
{
    delete m_widget;
}

}
}
