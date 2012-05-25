/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "gerritoptionspage.h"
#include "gerritparameters.h"

#include <coreplugin/icore.h>
#include <utils/pathchooser.h>

#include <QLineEdit>
#include <QSpinBox>
#include <QCheckBox>
#include <QFormLayout>
#include <QVBoxLayout>

namespace Gerrit {
namespace Internal {

GerritOptionsPage::GerritOptionsPage(const QSharedPointer<GerritParameters> &p,
                                     QObject *parent)
    : VcsBase::VcsBaseOptionsPage(parent)
    , m_parameters(p)
{
    setId(optionsId());
    setDisplayName(tr("Gerrit"));
}

QString GerritOptionsPage::optionsId()
{
    return QLatin1String("Gerrit");
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
        const GerritParameters newParameters = w->parameters();
        if (newParameters != *m_parameters) {
            *m_parameters = newParameters;
            m_parameters->setPortFlagBySshType();
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
    , m_portSpinBox(new QSpinBox(this))
    , m_additionalQueriesLineEdit(new QLineEdit(this))
    , m_httpsCheckBox(new QCheckBox(tr("HTTPS")))
{
    QFormLayout *formLayout = new QFormLayout(this);
    formLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    formLayout->addRow(tr("&Host: "), m_hostLineEdit);
    formLayout->addRow(tr("&User: "), m_userLineEdit);
    m_sshChooser->setExpectedKind(Utils::PathChooser::ExistingCommand);
    m_sshChooser->setCommandVersionArguments(QStringList(QLatin1String("-V")));
    formLayout->addRow(tr("&ssh: "), m_sshChooser);
    m_portSpinBox->setMinimum(1);
    m_portSpinBox->setMaximum(65535);
    formLayout->addRow(tr("&Port: "), m_portSpinBox);
    formLayout->addRow(tr("&Additional queries: "), m_additionalQueriesLineEdit);
    m_additionalQueriesLineEdit->setToolTip(tr(
    "A comma-separated list of additional queries.\n"
    "For example \"status:staged,status:integrating\""
    " can be used to show the status of the Continuous Integration\n"
    "of the Qt project."));
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
    result.port = m_portSpinBox->value();
    result.additionalQueries = m_additionalQueriesLineEdit->text().trimmed();
    result.https = m_httpsCheckBox->isChecked();
    return result;
}

void GerritOptionsWidget::setParameters(const GerritParameters &p)
{
    m_hostLineEdit->setText(p.host);
    m_userLineEdit->setText(p.user);
    m_sshChooser->setPath(p.ssh);
    m_portSpinBox->setValue(p.port);
    m_additionalQueriesLineEdit->setText(p.additionalQueries);
    m_httpsCheckBox->setChecked(p.https);
}

} // namespace Internal
} // namespace Gerrit
