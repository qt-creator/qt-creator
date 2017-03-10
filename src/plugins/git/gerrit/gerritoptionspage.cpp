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

#include "gerritoptionspage.h"
#include "gerritparameters.h"
#include "gerritserver.h"

#include <coreplugin/icore.h>
#include <utils/pathchooser.h>

#include <QLineEdit>
#include <QSpinBox>
#include <QCheckBox>
#include <QFormLayout>

namespace Gerrit {
namespace Internal {

GerritOptionsPage::GerritOptionsPage(const QSharedPointer<GerritParameters> &p,
                                     QObject *parent)
    : VcsBase::VcsBaseOptionsPage(parent)
    , m_parameters(p)
{
    setId("Gerrit");
    setDisplayName(tr("Gerrit"));
}

GerritOptionsPage::~GerritOptionsPage()
{
    delete m_widget;
}

QWidget *GerritOptionsPage::widget()
{
    if (!m_widget) {
        m_widget = new GerritOptionsWidget;
        m_widget->setParameters(*m_parameters);
    }
    return m_widget;
}

void GerritOptionsPage::apply()
{
    if (GerritOptionsWidget *w = m_widget.data()) {
        GerritParameters newParameters = w->parameters();
        if (newParameters != *m_parameters) {
            if (m_parameters->ssh == newParameters.ssh)
                newParameters.portFlag = m_parameters->portFlag;
            else
                newParameters.setPortFlagBySshType();
            *m_parameters = newParameters;
            m_parameters->toSettings(Core::ICore::settings());
        }
    }
}

void GerritOptionsPage::finish()
{
    delete m_widget;
}

GerritOptionsWidget::GerritOptionsWidget(QWidget *parent)
    : QWidget(parent)
    , m_hostLineEdit(new QLineEdit(this))
    , m_userLineEdit(new QLineEdit(this))
    , m_sshChooser(new Utils::PathChooser)
    , m_curlChooser(new Utils::PathChooser)
    , m_portSpinBox(new QSpinBox(this))
    , m_httpsCheckBox(new QCheckBox(tr("HTTPS")))
{
    QFormLayout *formLayout = new QFormLayout(this);
    formLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    formLayout->addRow(tr("&Host:"), m_hostLineEdit);
    formLayout->addRow(tr("&User:"), m_userLineEdit);
    m_sshChooser->setExpectedKind(Utils::PathChooser::ExistingCommand);
    m_sshChooser->setCommandVersionArguments({"-V"});
    m_sshChooser->setHistoryCompleter("Git.SshCommand.History");
    formLayout->addRow(tr("&ssh:"), m_sshChooser);
    m_curlChooser->setExpectedKind(Utils::PathChooser::ExistingCommand);
    m_curlChooser->setCommandVersionArguments({"-V"});
    formLayout->addRow(tr("cur&l:"), m_curlChooser);
    m_portSpinBox->setMinimum(1);
    m_portSpinBox->setMaximum(65535);
    formLayout->addRow(tr("SSH &Port:"), m_portSpinBox);
    formLayout->addRow(tr("P&rotocol:"), m_httpsCheckBox);
    m_httpsCheckBox->setToolTip(tr(
    "Determines the protocol used to form a URL in case\n"
    "\"canonicalWebUrl\" is not configured in the file\n"
    "\"gerrit.config\"."));
    setTabOrder(m_sshChooser, m_curlChooser);
    setTabOrder(m_curlChooser, m_portSpinBox);
}

GerritParameters GerritOptionsWidget::parameters() const
{
    GerritParameters result;
    result.server = GerritServer(m_hostLineEdit->text().trimmed(),
                                 static_cast<unsigned short>(m_portSpinBox->value()),
                                 m_userLineEdit->text().trimmed(),
                                 GerritServer::Ssh);
    result.ssh = m_sshChooser->path();
    result.curl = m_curlChooser->path();
    result.https = m_httpsCheckBox->isChecked();
    return result;
}

void GerritOptionsWidget::setParameters(const GerritParameters &p)
{
    m_hostLineEdit->setText(p.server.host);
    m_userLineEdit->setText(p.server.user.userName);
    m_sshChooser->setPath(p.ssh);
    m_curlChooser->setPath(p.curl);
    m_portSpinBox->setValue(p.server.port);
    m_httpsCheckBox->setChecked(p.https);
}

} // namespace Internal
} // namespace Gerrit
