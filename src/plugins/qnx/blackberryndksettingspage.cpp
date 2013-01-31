/**************************************************************************
**
** Copyright (C) 2011 - 2013 Research In Motion
**
** Contact: Research In Motion (blackberry-qt@qnx.com)
** Contact: KDAB (info@kdab.com)
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

#include "blackberryndksettingspage.h"
#include "blackberryndksettingswidget.h"
#include "qnxconstants.h"

#include <projectexplorer/projectexplorerconstants.h>

#include <QCoreApplication>

namespace Qnx {
namespace Internal {

BlackBerryNDKSettingsPage::BlackBerryNDKSettingsPage(QObject *parent) :
    Core::IOptionsPage(parent)
{
    setId(Core::Id(Constants::QNX_BB_NDK_SETTINGS_ID));
    setDisplayName(tr("NDK"));
    setCategory(Constants::QNX_BB_CATEGORY);
    setDisplayCategory(QCoreApplication::translate("BlackBerry",
                Constants::QNX_BB_CATEGORY_TR));
    setCategoryIcon(QLatin1String(Constants::QNX_BB_CATEGORY_ICON));
}

QWidget *BlackBerryNDKSettingsPage::createPage(QWidget *parent)
{
    m_widget = new BlackBerryNDKSettingsWidget(parent);
    return m_widget;
}

void BlackBerryNDKSettingsPage::apply()
{
}

void BlackBerryNDKSettingsPage::finish()
{
}

} // namespace Internal
} // namespace Qnx
