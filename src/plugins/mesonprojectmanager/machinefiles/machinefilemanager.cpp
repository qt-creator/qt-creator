/****************************************************************************
**
** Copyright (C) 2020 Alexis Jeandet.
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

#include "machinefilemanager.h"
#include "nativefilegenerator.h"
#include <kithelper/kitdata.h>
#include <kithelper/kithelper.h>
#include <coreplugin/icore.h>
#include <utils/optional.h>
#include <utils/qtcassert.h>
#include <QDir>
#include <QFile>
#include <QRegularExpression>

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
    return Utils::FilePath::fromString(Core::ICore::userResourcePath())
        .pathAppended("Meson-machine-files");
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
