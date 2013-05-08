/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
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

#include "gerritoptionspage.h"
#include "gerritparameters.h"

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

QWidget *GerritOptionsPage::createPage(QWidget *parent)
{
    GerritOptionsWidget *gow = new GerritOptionsWidget(parent);
    gow->setParameters(*m_parameters);
    m_widget = gow;
    return gow;
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
            m_parameters->toSettings(Core::ICore::instance()->settings());
        }
    }
}

bool GerritOptionsPage::matches(const QString &s) const
{
    return s.contains(QLatin1String("gerrit"), Qt::CaseInsensitive);
}

GerritOptionsWidget::GerritOptionsWidget(QWidget *parent)
    : QWidget(parent)
    , m_hostLineEdit(new QLineEdit(this))
    , m_userLineEdit(new QLineEdit(this))
    , m_sshChooser(new Utils::PathChooser)
    , m_repositoryChooser(new Utils::PathChooser)
    , m_portSpinBox(new QSpinBox(this))
    , m_httpsCheckBox(new QCheckBox(tr("HTTPS")))
    , m_promptPathCheckBox(new QCheckBox(tr("Always prompt for repository folder")))
{
    QFormLayout *formLayout = new QFormLayout(this);
    formLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    formLayout->addRow(tr("&Host:"), m_hostLineEdit);
    formLayout->addRow(tr("&User:"), m_userLineEdit);
    m_sshChooser->setExpectedKind(Utils::PathChooser::ExistingCommand);
    m_sshChooser->setCommandVersionArguments(QStringList(QLatin1String("-V")));
    formLayout->addRow(tr("&ssh:"), m_sshChooser);
    formLayout->addRow(tr("&Repository:"), m_repositoryChooser);
    m_repositoryChooser->setToolTip(tr("Default repository where patches will be applied."));
    formLayout->addRow(tr("Pr&ompt:"), m_promptPathCheckBox);
    m_promptPathCheckBox->setToolTip(tr("If checked, user will always be\n"
                                        "asked to confirm the repository path."));
    m_portSpinBox->setMinimum(1);
    m_portSpinBox->setMaximum(65535);
    formLayout->addRow(tr("&Port:"), m_portSpinBox);
    formLayout->addRow(tr("P&rotocol:"), m_httpsCheckBox);
    m_httpsCheckBox->setToolTip(tr(
    "Determines the protocol used to form a URL in case\n"
    "\"canonicalWebUrl\" is not configured in the file\n"
    "\"gerrit.config\"."));
}

GerritParameters GerritOptionsWidget::parameters() const
{
    GerritParameters result;
    result.host = m_hostLineEdit->text().trimmed();
    result.user = m_userLineEdit->text().trimmed();
    result.ssh = m_sshChooser->path();
    result.repositoryPath = m_repositoryChooser->path();
    result.port = m_portSpinBox->value();
    result.https = m_httpsCheckBox->isChecked();
    result.promptPath = m_promptPathCheckBox->isChecked();
    return result;
}

void GerritOptionsWidget::setParameters(const GerritParameters &p)
{
    m_hostLineEdit->setText(p.host);
    m_userLineEdit->setText(p.user);
    m_sshChooser->setPath(p.ssh);
    m_repositoryChooser->setPath(p.repositoryPath);
    m_portSpinBox->setValue(p.port);
    m_httpsCheckBox->setChecked(p.https);
    m_promptPathCheckBox->setChecked(p.promptPath);
}

} // namespace Internal
} // namespace Gerrit
