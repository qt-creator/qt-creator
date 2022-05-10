/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#include "cocoplugin.h"

#include "cocolanguageclient.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/icore.h>
#include <debugger/analyzer/analyzerconstants.h>
#include <utils/environment.h>
#include <utils/pathchooser.h>

#include <QAction>
#include <QDialogButtonBox>
#include <QFormLayout>

using namespace Utils;

namespace Coco {

class CocoPluginPrivate
{
public:
    void startCoco();

    CocoLanguageClient *m_client = nullptr;
};

void CocoPluginPrivate::startCoco()
{
    if (m_client)
        m_client->shutdown();
    m_client = nullptr;

    QDialog dialog(Core::ICore::dialogParent());
    dialog.setModal(true);
    auto layout = new QFormLayout();

    const Environment env = Environment::systemEnvironment();
    FilePath squishCocoPath = FilePath::fromString(env.value("SQUISHCOCO"));
    const FilePaths candidates = env.findAllInPath("coveragebrowser", {squishCocoPath});

    PathChooser cocoChooser;
    if (!candidates.isEmpty())
        cocoChooser.setFilePath(candidates.first());
    cocoChooser.setExpectedKind(PathChooser::Command);
    cocoChooser.setPromptDialogTitle(CocoPlugin::tr(
                                             "Select a Squish Coco CoverageBrowser Executable"));

    cocoChooser.setHistoryCompleter("Coco.CoverageBrowser.history", true);
    layout->addRow(CocoPlugin::tr("CoverageBrowser:"), &cocoChooser);
    PathChooser csmesChoser;
    csmesChoser.setHistoryCompleter("Coco.CSMes.history", true);
    csmesChoser.setExpectedKind(PathChooser::File);
    csmesChoser.setInitialBrowsePathBackup(FileUtils::homePath());
    csmesChoser.setPromptDialogFilter(CocoPlugin::tr("Coco instrumentation files (*.csmes)"));
    csmesChoser.setPromptDialogTitle(CocoPlugin::tr("Select a Squish Coco Instrumentation File"));
    layout->addRow(CocoPlugin::tr("CSMes:"), &csmesChoser);
    QDialogButtonBox buttons(QDialogButtonBox::Cancel | QDialogButtonBox::Open);
    layout->addWidget(&buttons);
    dialog.setLayout(layout);

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

CocoPlugin::CocoPlugin()
    : d (new CocoPluginPrivate)
{}

CocoPlugin::~CocoPlugin()
{
    delete d;
}

bool CocoPlugin::initialize(const QStringList &, QString *)
{
    using namespace Core;
    ActionContainer *menu = ActionManager::actionContainer(Debugger::Constants::M_DEBUG_ANALYZER);
    if (menu) {
        auto startCoco = new QAction("Squish Coco ...", this);
        Command *cmd = ActionManager::registerAction(startCoco, "Coco.startCoco");
        menu->addAction(cmd, Debugger::Constants::G_ANALYZER_TOOLS);

        connect(startCoco, &QAction::triggered, this, [this]() { d->startCoco(); });
    }
    return true;
}

} // namespace Coco


