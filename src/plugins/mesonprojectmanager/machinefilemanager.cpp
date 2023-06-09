// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "machinefilemanager.h"

#include "kitdata.h"
#include "kithelper.h"

#include <coreplugin/icore.h>

#include <projectexplorer/kitinformation.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/toolchain.h>

#include <utils/macroexpander.h>
#include <utils/qtcassert.h>

#include <optional>

using namespace ProjectExplorer;
using namespace Utils;

namespace MesonProjectManager::Internal {

const char MACHINE_FILE_PREFIX[] = "Meson-MachineFile-";
const char MACHINE_FILE_EXT[] = ".ini";

static FilePath machineFilesDir()
{
    return Core::ICore::userResourcePath("Meson-machine-files");
}

MachineFileManager::MachineFileManager()
{
    connect(KitManager::instance(), &KitManager::kitAdded,
            this, &MachineFileManager::addMachineFile);
    connect(KitManager::instance(), &KitManager::kitUpdated,
            this, &MachineFileManager::updateMachineFile);
    connect(KitManager::instance(), &KitManager::kitRemoved,
            this, &MachineFileManager::removeMachineFile);
    connect(KitManager::instance(), &KitManager::kitsLoaded,
            this, &MachineFileManager::cleanupMachineFiles);
}

FilePath MachineFileManager::machineFile(const Kit *kit)
{
    QTC_ASSERT(kit, return {});
    auto baseName
        = QString("%1%2%3").arg(MACHINE_FILE_PREFIX).arg(kit->id().toString()).arg(MACHINE_FILE_EXT);
    baseName = baseName.remove('{').remove('}');
    return machineFilesDir().pathAppended(baseName);
}

void MachineFileManager::addMachineFile(const Kit *kit)
{
    FilePath filePath = machineFile(kit);
    QTC_ASSERT(!filePath.isEmpty(), return );
    auto kitData = KitHelper::kitData(kit);

    auto entry = [](const QString &key, const QString &value) {
        return QString("%1 = '%2'\n").arg(key).arg(value).toUtf8();
    };

    QByteArray ba = "[binaries]\n";
    ba += entry("c", kitData.cCompilerPath);
    ba += entry("cpp", kitData.cxxCompilerPath);
    ba += entry("qmake", kitData.qmakePath);
    if (kitData.qtVersion == QtMajorVersion::Qt4)
        ba += entry("qmake-qt4", kitData.qmakePath);
    else if (kitData.qtVersion == QtMajorVersion::Qt5)
        ba += entry("qmake-qt5", kitData.qmakePath);
    else if (kitData.qtVersion == QtMajorVersion::Qt6)
        ba += entry("qmake-qt6", kitData.qmakePath);
    ba += entry("cmake", kitData.cmakePath);

    filePath.writeFileContents(ba);
}

void MachineFileManager::removeMachineFile(const Kit *kit)
{
    FilePath filePath = machineFile(kit);
    if (filePath.exists())
        filePath.removeFile();
}

void MachineFileManager::updateMachineFile(const Kit *kit)
{
    addMachineFile(kit);
}

void MachineFileManager::cleanupMachineFiles()
{
    FilePath dir = machineFilesDir();
    dir.ensureWritableDir();

    const FileFilter filter = {{QString("%1*%2").arg(MACHINE_FILE_PREFIX).arg(MACHINE_FILE_EXT)}};
    const FilePaths machineFiles = dir.dirEntries(filter);

    FilePaths expected;
    for (Kit const *kit : KitManager::kits()) {
        const FilePath fname = machineFile(kit);
        expected.push_back(fname);
        if (!machineFiles.contains(fname))
            addMachineFile(kit);
    }

    for (const FilePath &file : machineFiles) {
        if (!expected.contains(file))
            file.removeFile();
    }
}

} // MesonProjectManager::Internal
