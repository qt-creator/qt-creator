// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "machinefilemanager.h"

#include "kitdata.h"
#include "kithelper.h"
#include "nativefilegenerator.h"

#include <coreplugin/icore.h>

#include <utils/qtcassert.h>

#include <QDir>
#include <QFile>
#include <QRegularExpression>

#include <optional>

namespace MesonProjectManager {
namespace Internal {

const char MACHINE_FILE_PREFIX[] = "Meson-MachineFile-";
const char MACHINE_FILE_EXT[] = ".ini";

template<typename F>
bool withFile(const Utils::FilePath &path, const F &f)
{
    QFile file(path.toString());
    if (file.open(QIODevice::WriteOnly)) {
        f(&file);
        return file.flush();
    }
    return false;
}

Utils::FilePath MachineFilesDir()
{
    return Core::ICore::userResourcePath("Meson-machine-files");
}

MachineFileManager::MachineFileManager()
{
    using namespace ProjectExplorer;
    connect(KitManager::instance(),
            &KitManager::kitAdded,
            this,
            &MachineFileManager::addMachineFile);
    connect(KitManager::instance(),
            &KitManager::kitUpdated,
            this,
            &MachineFileManager::updateMachineFile);
    connect(KitManager::instance(),
            &KitManager::kitRemoved,
            this,
            &MachineFileManager::removeMachineFile);
    connect(KitManager::instance(),
            &KitManager::kitsLoaded,
            this,
            &MachineFileManager::cleanupMachineFiles);
}

Utils::FilePath MachineFileManager::machineFile(const ProjectExplorer::Kit *kit)
{
    QTC_ASSERT(kit, return {});
    auto baseName
        = QString("%1%2%3").arg(MACHINE_FILE_PREFIX).arg(kit->id().toString()).arg(MACHINE_FILE_EXT);
    baseName = baseName.remove('{').remove('}');
    return MachineFilesDir().pathAppended(baseName);
}

void MachineFileManager::addMachineFile(const ProjectExplorer::Kit *kit)
{
    auto filePath = machineFile(kit);
    QTC_ASSERT(!filePath.isEmpty(), return );
    auto data = KitHelper::kitData(kit);
    QTC_ASSERT(withFile(filePath,
                        [&data](QFile *file) { NativeFileGenerator::makeNativeFile(file, data); }),
               return );
}

void MachineFileManager::removeMachineFile(const ProjectExplorer::Kit *kit)
{
    auto filePath = machineFile(kit);
    if (filePath.exists())
        QFile::remove(filePath.toString());
}

void MachineFileManager::updateMachineFile(const ProjectExplorer::Kit *kit)
{
    addMachineFile(kit);
}

void MachineFileManager::cleanupMachineFiles()
{
    const auto kits = ProjectExplorer::KitManager::kits();
    auto machineFilesDir = QDir(MachineFilesDir().toString());
    if (!machineFilesDir.exists()) {
        machineFilesDir.mkdir(machineFilesDir.path());
    }
    auto machineFiles = QDir(MachineFilesDir().toString())
                            .entryList(
                                {QString("%1*%2").arg(MACHINE_FILE_PREFIX).arg(MACHINE_FILE_EXT)});
    QStringList expected;
    for (auto const *kit : kits) {
        QString fname = machineFile(kit).toString();
        expected.push_back(fname);
        if (!machineFiles.contains(fname))
            addMachineFile(kit);
    }

    for (const auto &file : machineFiles) {
        if (!expected.contains(file))
            QFile::remove(file);
    }
}

} // namespace Internal
} // namespace MesonProjectManager
