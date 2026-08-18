// Copyright (C) 2019 Klarälvdalens Datakonsult AB, a KDAB Group company,
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "ctfvisualizertool.h"

#include "ctfvisualizerconstants.h"
#include "profilertr.h"
#include "profilertraceeditor.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>

#include <utils/fileutils.h>
#include <utils/shutdownguard.h>

#include <QAction>
#include <QFileDialog>
#include <QMenu>

using namespace Core;
using namespace Utils;

namespace Profiler::Internal {

// Opens Chrome Trace Format and Common Trace Format traces. The views live in
// the trace editor; this only offers the two entry points, because neither
// format has a mime type that could be routed to an editor factory.
class CtfVisualizerTool : public QObject
{
public:
    CtfVisualizerTool();

private:
    QAction m_loadJson;
    QAction m_loadCtf2;
};

CtfVisualizerTool::CtfVisualizerTool()
{
    ActionContainer *menu = ActionManager::actionContainer(Core::Constants::M_DEBUG_ANALYZER);
    ActionContainer *options = ActionManager::createMenu(Constants::CtfVisualizerMenuId);
    options->menu()->setTitle(Tr::tr("Chrome Trace Format Viewer"));
    menu->addMenu(options, Core::Constants::G_ANALYZER_REMOTE_TOOLS);
    options->menu()->setEnabled(true);

    const Context globalContext(Core::Constants::C_GLOBAL);

    m_loadJson.setText(Tr::tr("Load JSON File"));
    options->addAction(
        ActionManager::registerAction(&m_loadJson, Constants::CtfVisualizerTaskLoadJson,
                                      globalContext));
    connect(&m_loadJson, &QAction::triggered, this, [this] {
        // The action carries a path when it is triggered programmatically.
        QString fileName = m_loadJson.data().toString();
        if (fileName.isEmpty()) {
            fileName = QFileDialog::getOpenFileName(ICore::dialogParent(),
                                                    Tr::tr("Load Chrome Trace Format File"), {},
                                                    Tr::tr("JSON File (*.json)"));
        }
        if (!fileName.isEmpty())
            openTraceFile(FilePath::fromUserInput(fileName));
    });

    m_loadCtf2.setText(Tr::tr("Load CTF2 Trace"));
    options->addAction(
        ActionManager::registerAction(&m_loadCtf2, Constants::CtfVisualizerTaskLoadCtf2,
                                      globalContext));
    connect(&m_loadCtf2, &QAction::triggered, this, [this] {
        QString dirPath = m_loadCtf2.data().toString();
        if (dirPath.isEmpty()) {
            dirPath = QFileDialog::getExistingDirectory(ICore::dialogParent(),
                                                        Tr::tr("Load CTF2 Trace Directory"));
        }
        if (!dirPath.isEmpty())
            openTraceFile(FilePath::fromUserInput(dirPath));
    });
}

void setupCtfVisualizerTool()
{
    static GuardedObject<CtfVisualizerTool> theTool;
}

} // namespace Profiler::Internal
