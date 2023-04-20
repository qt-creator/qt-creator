// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "gerritoptionspage.h"
#include "gerritparameters.h"
#include "gerritserver.h"
#include "../gittr.h"

#include <coreplugin/icore.h>
#include <utils/pathchooser.h>

#include <vcsbase/vcsbaseconstants.h>

#include <QLineEdit>
#include <QSpinBox>
#include <QCheckBox>
#include <QFormLayout>

namespace Gerrit::Internal {

class GerritOptionsWidget : public Core::IOptionsPageWidget
{
public:
    GerritOptionsWidget(GerritOptionsPage *page, const QSharedPointer<GerritParameters> &p)
        : m_page(page)
        , m_hostLineEdit(new QLineEdit(this))
        , m_userLineEdit(new QLineEdit(this))
        , m_sshChooser(new Utils::PathChooser)
        , m_curlChooser(new Utils::PathChooser)
        , m_portSpinBox(new QSpinBox(this))
        , m_httpsCheckBox(new QCheckBox(Git::Tr::tr("HTTPS")))
        , m_parameters(p)
    {
        m_hostLineEdit->setText(p->server.host);
        m_userLineEdit->setText(p->server.user.userName);
        m_sshChooser->setFilePath(p->ssh);
        m_curlChooser->setFilePath(p->curl);
        m_portSpinBox->setValue(p->server.port);
        m_httpsCheckBox->setChecked(p->https);

        auto formLayout = new QFormLayout(this);
        formLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
        formLayout->addRow(Git::Tr::tr("&Host:"), m_hostLineEdit);
        formLayout->addRow(Git::Tr::tr("&User:"), m_userLineEdit);
        m_sshChooser->setExpectedKind(Utils::PathChooser::ExistingCommand);
        m_sshChooser->setCommandVersionArguments({"-V"});
        m_sshChooser->setHistoryCompleter("Git.SshCommand.History");
        formLayout->addRow(Git::Tr::tr("&ssh:"), m_sshChooser);
        m_curlChooser->setExpectedKind(Utils::PathChooser::ExistingCommand);
        m_curlChooser->setCommandVersionArguments({"-V"});
        formLayout->addRow(Git::Tr::tr("cur&l:"), m_curlChooser);
        m_portSpinBox->setMinimum(1);
        m_portSpinBox->setMaximum(65535);
        formLayout->addRow(Git::Tr::tr("SSH &Port:"), m_portSpinBox);
        formLayout->addRow(Git::Tr::tr("P&rotocol:"), m_httpsCheckBox);
        m_httpsCheckBox->setToolTip(Git::Tr::tr(
            "Determines the protocol used to form a URL in case\n"
            "\"canonicalWebUrl\" is not configured in the file\n"
            "\"gerrit.config\"."));
        setTabOrder(m_sshChooser, m_curlChooser);
        setTabOrder(m_curlChooser, m_portSpinBox);
    }

private:
    void apply() final;

    GerritOptionsPage *m_page;
    QLineEdit *m_hostLineEdit;
    QLineEdit *m_userLineEdit;
    Utils::PathChooser *m_sshChooser;
    Utils::PathChooser *m_curlChooser;
    QSpinBox *m_portSpinBox;
    QCheckBox *m_httpsCheckBox;
    const QSharedPointer<GerritParameters> &m_parameters;
};

void GerritOptionsWidget::apply()
{
    GerritParameters newParameters;
    newParameters.server = GerritServer(m_hostLineEdit->text().trimmed(),
                                 static_cast<unsigned short>(m_portSpinBox->value()),
                                 m_userLineEdit->text().trimmed(),
                                 GerritServer::Ssh);
    newParameters.ssh = m_sshChooser->filePath();
    newParameters.curl = m_curlChooser->filePath();
    newParameters.https = m_httpsCheckBox->isChecked();

    if (newParameters != *m_parameters) {
        if (m_parameters->ssh == newParameters.ssh)
            newParameters.portFlag = m_parameters->portFlag;
        else
            newParameters.setPortFlagBySshType();
        *m_parameters = newParameters;
        m_parameters->toSettings(Core::ICore::settings());
        emit m_page->settingsChanged();
    }
}

// GerritOptionsPage

GerritOptionsPage::GerritOptionsPage(const QSharedPointer<GerritParameters> &p, QObject *parent)
    : Core::IOptionsPage(parent)
{
    setId("Gerrit");
    setDisplayName(Git::Tr::tr("Gerrit"));
    setCategory(VcsBase::Constants::VCS_SETTINGS_CATEGORY);
    setWidgetCreator([this, p] { return new GerritOptionsWidget(this, p); });
}

} // Gerrit::Internal
