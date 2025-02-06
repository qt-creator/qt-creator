// Copyright (c) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "haskellmanager.h"

#include "haskellconstants.h"
#include "haskellsettings.h"
#include "haskelltr.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/editormanager/editormanager.h>

#include <utils/algorithm.h>
#include <utils/commandline.h>
#include <utils/mimeutils.h>
#include <utils/qtcprocess.h>

using namespace Core;
using namespace Utils;

namespace Haskell::Internal {

void openGhci(const FilePath &haskellFile)
{
    const QList<MimeType> mimeTypes = mimeTypesForFileName(haskellFile.toUrlishString());
    const bool isHaskell = Utils::anyOf(mimeTypes, [](const MimeType &mt) {
        return mt.inherits("text/x-haskell") || mt.inherits("text/x-literate-haskell");
    });
    const auto args = QStringList{"ghci"}
                      + (isHaskell ? QStringList{haskellFile.fileName()} : QStringList());
    Process p;
    p.setTerminalMode(TerminalMode::Detached);
    p.setCommand({settings().stackPath(), args});
    p.setWorkingDirectory(haskellFile.absolutePath());
    p.start();
}

void setupHaskellActions(QObject *guard)
{
    ActionBuilder runGhci(guard, Haskell::Constants::A_RUN_GHCI);
    runGhci.setText(Tr::tr("Run GHCi"));
    runGhci.addOnTriggered(guard, [] {
        if (IDocument *doc = EditorManager::currentDocument())
            openGhci(doc->filePath());
    });
}

} // Haskell::Internal
