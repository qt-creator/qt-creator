// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cocolanguageclient.h"
#include "cocotr.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/icore.h>

#include <debugger/analyzer/analyzerconstants.h>

#include <extensionsystem/iplugin.h>

#include <utils/environment.h>
#include <utils/fileutils.h>
#include <utils/pathchooser.h>

#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>

using namespace Core;
using namespace Utils;

namespace Coco {

class CocoPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "Coco.json")

public:
    ~CocoPlugin() final
    {
        // FIXME: Kill m_client?
    }

    void initialize() final
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

        QDialog dialog(ICore::dialogParent());
        dialog.setModal(true);
        auto layout = new QFormLayout();

        const Environment env = Environment::systemEnvironment();
        const FilePath squishCocoPath = FilePath::fromUserInput(env.value("SQUISHCOCO"));
        const FilePath candidate = FilePath("coveragebrowser").searchInPath({squishCocoPath},
                                                                            FilePath::PrependToPath);

        PathChooser cocoChooser;
        if (!candidate.isEmpty())
            cocoChooser.setFilePath(candidate);
        cocoChooser.setExpectedKind(PathChooser::Command);
        cocoChooser.setPromptDialogTitle(Tr::tr("Select a Squish Coco CoverageBrowser Executable"));

        cocoChooser.setHistoryCompleter("Coco.CoverageBrowser.history", true);
        layout->addRow(Tr::tr("CoverageBrowser:"), &cocoChooser);
        PathChooser csmesChoser;
        csmesChoser.setHistoryCompleter("Coco.CSMes.history", true);
        csmesChoser.setExpectedKind(PathChooser::File);
        csmesChoser.setInitialBrowsePathBackup(FileUtils::homePath());
        csmesChoser.setPromptDialogFilter(Tr::tr("Coco instrumentation files (*.csmes)"));
        csmesChoser.setPromptDialogTitle(Tr::tr("Select a Squish Coco Instrumentation File"));
        layout->addRow(Tr::tr("CSMes:"), &csmesChoser);
        QDialogButtonBox buttons(QDialogButtonBox::Cancel | QDialogButtonBox::Open);
        layout->addItem(new QSpacerItem(0, 20, QSizePolicy::Expanding, QSizePolicy::MinimumExpanding));
        layout->addWidget(&buttons);
        dialog.setLayout(layout);
        dialog.resize(480, dialog.height());

        QObject::connect(&buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
        QObject::connect(&buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

        if (dialog.exec() == QDialog::Accepted) {
            const FilePath cocoPath = cocoChooser.filePath();
            const FilePath csmesPath = csmesChoser.filePath();
            if (cocoPath.isExecutableFile() && csmesPath.exists()) {
                m_client = new CocoLanguageClient(cocoPath, csmesPath);
                m_client->start();
            }
        }
    }

private:
    CocoLanguageClient *m_client = nullptr;
};

} // namespace Coco

#include "cocoplugin.moc"
