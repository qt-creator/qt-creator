/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (C) 2011 - 2012 Research In Motion
**
** Contact: Research In Motion (blackberry-qt@qnx.com)
** Contact: KDAB (info@kdab.com)
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
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "qnxbaseqtconfigwidget.h"
#include "ui_qnxbaseqtconfigwidget.h"

#include "qnxabstractqtversion.h"

#include <utils/pathchooser.h>

#include <QDir>
#include <QFormLayout>

using namespace Qnx;
using namespace Qnx::Internal;

QnxBaseQtConfigWidget::QnxBaseQtConfigWidget(QnxAbstractQtVersion *version)
    : QtSupport::QtConfigWidget()
    , m_version(version)
{
    m_ui = new Ui::QnxBaseQtConfigWidget;
    m_ui->setupUi(this);

    m_ui->sdkLabel->setText(version->sdkDescription());

    m_ui->sdkPath->setExpectedKind(Utils::PathChooser::ExistingDirectory);
    m_ui->sdkPath->setPath(version->sdkPath());

    connect(m_ui->sdkPath, SIGNAL(changed(QString)), this, SLOT(updateSdkPath(QString)));
}

QnxBaseQtConfigWidget::~QnxBaseQtConfigWidget()
{
    delete m_ui;
    m_ui = 0;
}

void QnxBaseQtConfigWidget::updateSdkPath(const QString &path)
{
    m_version->setSdkPath(path);
    emit changed();
}
