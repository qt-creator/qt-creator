/****************************************************************************
**
** Copyright (C) 2016 BlackBerry Limited. All rights reserved.
** Contact: KDAB (info@kdab.com)
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

#include "qnxbaseqtconfigwidget.h"

#include "qnxqtversion.h"

#include <utils/pathchooser.h>
#include <utils/qtcassert.h>

#include <QDir>
#include <QHBoxLayout>

namespace Qnx {
namespace Internal {

QnxBaseQtConfigWidget::QnxBaseQtConfigWidget(QnxQtVersion *version) :
    m_version(version),
    m_sdkPathChooser(new Utils::PathChooser)
{
    QTC_ASSERT(version, return);

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->addWidget(m_sdkPathChooser);

    m_sdkPathChooser->setExpectedKind(Utils::PathChooser::ExistingDirectory);
    m_sdkPathChooser->setHistoryCompleter(QLatin1String("Qnx.Sdk.History"));
    m_sdkPathChooser->setPath(version->sdkPath());

    connect(m_sdkPathChooser, &Utils::PathChooser::rawPathChanged,
            this, &QnxBaseQtConfigWidget::updateSdkPath);
}

void QnxBaseQtConfigWidget::updateSdkPath(const QString &path)
{
    m_version->setSdkPath(path);
    emit changed();
}

} // namespace Internal
} // namespace Qnx
