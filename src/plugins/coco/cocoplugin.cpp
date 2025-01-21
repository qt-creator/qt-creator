// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cocobuildstep.h"
#include "cocolanguageclient.h"
#include "cocopluginconstants.h"
#include "cocoprojectsettingswidget.h"
#include "cocotr.h"
#include "globalsettings.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/icore.h>

#include <debugger/debuggerconstants.h>

#include <extensionsystem/iplugin.h>

#include <projectexplorer/project.h>
#include <projectexplorer/projectpanelfactory.h>
#include <projectexplorer/target.h>

#include <utils/environment.h>
#include <utils/fileutils.h>
#include <utils/pathchooser.h>

#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QMessageBox>
#include <QPushButton>

using namespace Core;
using namespace ProjectExplorer;
using namespace Utils;

namespace Coco::Internal {

class CocoPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "Coco.json")

public:
    ~CocoPlugin() final
    {
        // FIXME: Kill m_client?
    }

    void initLanguageServer()
    {
        ActionBuilder(this, "Coco.startCoco")
            .setText("Squish Coco ...")
            .addToContainer(Debugger::Constants::M_DEBUG_ANALYZER,  Debugger::Constants::G_ANALYZER_TOOLS)
            .addOnTriggered(this, &CocoPlugin::startCoco);
    }

    void startCoco()
    {
        if (m_client)
            m_client->shutdown();
        m_client = nullptr;

        if (cocoSettings().isValid()) {
            QDialog dialog(ICore::dialogParent());
            dialog.setModal(true);
            auto layout = new QFormLayout();

            PathChooser csmesChoser;
            csmesChoser.setHistoryCompleter("Coco.CSMes.history", true);
            csmesChoser.setExpectedKind(PathChooser::File);
            csmesChoser.setInitialBrowsePathBackup(PathChooser::homePath());
            csmesChoser.setPromptDialogFilter(Tr::tr("Coco instrumentation files (*.csmes)"));
            csmesChoser.setPromptDialogTitle(Tr::tr("Select a Squish Coco Instrumentation File"));
            layout->addRow(Tr::tr("CSMes file:"), &csmesChoser);
            QDialogButtonBox buttons(QDialogButtonBox::Cancel | QDialogButtonBox::Open);
            layout->addWidget(&buttons);
            dialog.setLayout(layout);
            dialog.resize(480, dialog.height());

            QObject::connect(&buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
            QObject::connect(&buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

            if (dialog.exec() == QDialog::Accepted) {
                const FilePath cocoPath = cocoSettings().coverageBrowserPath();
                const FilePath csmesPath = csmesChoser.filePath();
                if (cocoPath.isExecutableFile() && csmesPath.exists()) {
                    m_client = new CocoLanguageClient(cocoPath, csmesPath);
                    m_client->start();
                }
            }
        } else {
            QMessageBox msg;
            msg.setText(Tr::tr("No valid CoverageScanner found."));
            QPushButton *configButton = msg.addButton(Tr::tr("Configure"), QMessageBox::AcceptRole);
            msg.setStandardButtons(QMessageBox::Cancel);
            msg.exec();

            if (msg.clickedButton() == configButton)
                Core::ICore::showOptionsDialog(Constants::COCO_SETTINGS_PAGE_ID);
        }
    }

    void initialize() final;

private:
    CocoLanguageClient *m_client = nullptr;
};

void CocoPlugin::initialize()
{
    setupCocoBuildSteps();

    IOptionsPage::registerCategory(
        "I.Coco",
        QCoreApplication::translate("Coco", "Coco"),
        ":/cocoplugin/images/SquishCoco_48x48.png");

    setupCocoSettings();

    setupCocoProjectPanel();

    initLanguageServer();
}

} // namespace Coco::Internal

#include "cocoplugin.moc"
