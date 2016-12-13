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
    m_sdpPathChooser(new Utils::PathChooser)
{
    QTC_ASSERT(version, return);

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->addWidget(m_sdpPathChooser);

    m_sdpPathChooser->setExpectedKind(Utils::PathChooser::ExistingDirectory);
    m_sdpPathChooser->setHistoryCompleter(QLatin1String("Qnx.Sdp.History"));
    m_sdpPathChooser->setPath(version->sdpPath());

    connect(m_sdpPathChooser, &Utils::PathChooser::rawPathChanged,
            this, &QnxBaseQtConfigWidget::updateSdpPath);
}

void QnxBaseQtConfigWidget::updateSdpPath(const QString &path)
{
    m_version->setSdpPath(path);
    emit changed();
}

} // namespace Internal
} // namespace Qnx
