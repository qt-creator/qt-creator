// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "gerritoptionspage.h"
#include "gerritparameters.h"
#include "gerritserver.h"
#include "../gittr.h"

#include <coreplugin/icore.h>

#include <utils/layoutbuilder.h>
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
    GerritOptionsWidget(const std::function<void()> &onChanged)
    {
        const GerritParameters &s = gerritSettings();
        auto hostLineEdit = new QLineEdit(s.server.host);

        auto userLineEdit = new QLineEdit(s.server.user.userName);

        auto sshChooser = new Utils::PathChooser;
        sshChooser->setFilePath(s.ssh);
        sshChooser->setExpectedKind(Utils::PathChooser::ExistingCommand);
        sshChooser->setCommandVersionArguments({"-V"});
        sshChooser->setHistoryCompleter("Git.SshCommand.History");

        auto curlChooser = new Utils::PathChooser;
        curlChooser->setFilePath(s.curl);
        curlChooser->setExpectedKind(Utils::PathChooser::ExistingCommand);
        curlChooser->setCommandVersionArguments({"-V"});

        auto portSpinBox = new QSpinBox(this);
        portSpinBox->setRange(1, 65535);
        portSpinBox->setValue(s.server.port);

        auto httpsCheckBox = new QCheckBox(Git::Tr::tr("HTTPS"));
        httpsCheckBox->setChecked(s.https);
        httpsCheckBox->setToolTip(Git::Tr::tr(
            "Determines the protocol used to form a URL in case\n"
            "\"canonicalWebUrl\" is not configured in the file\n"
            "\"gerrit.config\"."));

        using namespace Layouting;
        Form {
            Git::Tr::tr("&Host:"), hostLineEdit, br,
            Git::Tr::tr("&User:"), userLineEdit, br,
            Git::Tr::tr("&ssh:"), sshChooser, br,
            Git::Tr::tr("cur&l:"), curlChooser, br,
            Git::Tr::tr("SSH &Port:"), portSpinBox, br,
            Git::Tr::tr("P&rotocol:"), httpsCheckBox
        }.attachTo(this);

        setOnApply([hostLineEdit,
                    userLineEdit,
                    sshChooser,
                    curlChooser,
                    portSpinBox,
                    httpsCheckBox,
                    onChanged] {
            GerritParameters &s = gerritSettings();
            GerritParameters newParameters;
            newParameters.server = GerritServer(hostLineEdit->text().trimmed(),
                                         static_cast<unsigned short>(portSpinBox->value()),
                                         userLineEdit->text().trimmed(),
                                         GerritServer::Ssh);
            newParameters.ssh = sshChooser->filePath();
            newParameters.curl = curlChooser->filePath();
            newParameters.https = httpsCheckBox->isChecked();

            if (newParameters != s) {
                if (s.ssh == newParameters.ssh)
                    newParameters.portFlag = s.portFlag;
                else
                    newParameters.setPortFlagBySshType();
                s = newParameters;
                s.toSettings();
                emit onChanged();
            }
        });
    }
};

// GerritOptionsPage

GerritOptionsPage::GerritOptionsPage(const std::function<void()> &onChanged)
{
    setId("Gerrit");
    setDisplayName(Git::Tr::tr("Gerrit"));
    setCategory(VcsBase::Constants::VCS_SETTINGS_CATEGORY);
    setWidgetCreator([onChanged] { return new GerritOptionsWidget(onChanged); });
}

} // Gerrit::Internal
