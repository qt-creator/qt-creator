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

#include "bardescriptoreditorauthorinformationwidget.h"
#include "ui_bardescriptoreditorauthorinformationwidget.h"

#include "blackberrydebugtokenreader.h"
#include "blackberrydeviceconfiguration.h"
#include "qnxconstants.h"

#include <projectexplorer/devicesupport/devicemanager.h>
#include <utils/qtcassert.h>

#include <QInputDialog>
#include <QMessageBox>

using namespace Qnx;
using namespace Qnx::Internal;

BarDescriptorEditorAuthorInformationWidget::BarDescriptorEditorAuthorInformationWidget(QWidget *parent) :
    BarDescriptorEditorAbstractPanelWidget(parent),
    m_ui(new Ui::BarDescriptorEditorAuthorInformationWidget)
{
    m_ui->setupUi(this);

    m_ui->setFromDebugToken->setVisible(BlackBerryDebugTokenReader::isSupported());

    connect(m_ui->author, SIGNAL(textChanged(QString)), this, SIGNAL(changed()));
    connect(m_ui->authorId, SIGNAL(textChanged(QString)), this, SIGNAL(changed()));
    connect(m_ui->setFromDebugToken, SIGNAL(clicked()), this, SLOT(setAuthorFromDebugToken()));
}

BarDescriptorEditorAuthorInformationWidget::~BarDescriptorEditorAuthorInformationWidget()
{
    delete m_ui;
}

void BarDescriptorEditorAuthorInformationWidget::clear()
{
    setLineEditBlocked(m_ui->author, QString());
    setLineEditBlocked(m_ui->authorId, QString());
}

QString BarDescriptorEditorAuthorInformationWidget::author() const
{
    return m_ui->author->text();
}

void BarDescriptorEditorAuthorInformationWidget::setAuthor(const QString &author)
{
    setLineEditBlocked(m_ui->author, author);
}

QString BarDescriptorEditorAuthorInformationWidget::authorId() const
{
    return m_ui->authorId->text();
}

void BarDescriptorEditorAuthorInformationWidget::setAuthorId(const QString &authorId)
{
    setLineEditBlocked(m_ui->authorId, authorId);
}

void BarDescriptorEditorAuthorInformationWidget::setAuthorFromDebugToken()
{
    // To select debug token, make it fancier once the debug token management is done in
    // Qt Creator
    QStringList debugTokens;
    ProjectExplorer::DeviceManager *deviceManager = ProjectExplorer::DeviceManager::instance();
    for (int i = 0; i < deviceManager->deviceCount(); ++i) {
        ProjectExplorer::IDevice::ConstPtr device = deviceManager->deviceAt(i);
        if (device->type() == Core::Id(Constants::QNX_BB_OS_TYPE)) {
            BlackBerryDeviceConfiguration::ConstPtr bbDevice = device.dynamicCast<const BlackBerryDeviceConfiguration>();
            QTC_ASSERT(bbDevice, continue);

            debugTokens << bbDevice->debugToken();
        }
    }
    debugTokens.removeDuplicates();

    bool ok;
    QString debugToken = QInputDialog::getItem(this, tr("Select Debug Token"), tr("Debug token:"), debugTokens, 0, false, &ok);
    if (!ok || debugToken.isEmpty())
        return;

    BlackBerryDebugTokenReader debugTokenReader(debugToken);
    if (!debugTokenReader.isValid()) {
        QMessageBox::warning(this, tr("Error Reading Debug Token"), tr("There was a problem reading debug token."));
        return;
    }

    m_ui->author->setText(debugTokenReader.author());
    m_ui->authorId->setText(debugTokenReader.authorId());
}
