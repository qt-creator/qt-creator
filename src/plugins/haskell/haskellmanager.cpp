// Copyright (c) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "haskellmanager.h"

#include "haskelltr.h"

#include <coreplugin/messagemanager.h>
#include <utils/algorithm.h>
#include <utils/commandline.h>
#include <utils/hostosinfo.h>
#include <utils/mimeutils.h>
#include <utils/process.h>
#include <utils/processenums.h>

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QSettings>

#include <unordered_map>

static const char kStackExecutableKey[] = "Haskell/StackExecutable";

using namespace Utils;

namespace Haskell {
namespace Internal {

class HaskellManagerPrivate
{
public:
    FilePath stackExecutable;
};

Q_GLOBAL_STATIC(HaskellManagerPrivate, m_d)
Q_GLOBAL_STATIC(HaskellManager, m_instance)

HaskellManager *HaskellManager::instance()
{
    return m_instance;
}

FilePath HaskellManager::findProjectDirectory(const FilePath &filePath)
{
    if (filePath.isEmpty())
        return {};

    QDir directory(filePath.toFileInfo().isDir() ? filePath.toString()
                                                 : filePath.parentDir().toString());
    directory.setNameFilters({"stack.yaml", "*.cabal"});
    directory.setFilter(QDir::Files | QDir::Readable);
    do {
        if (!directory.entryList().isEmpty())
            return FilePath::fromString(directory.path());
    } while (!directory.isRoot() && directory.cdUp());
    return {};
}

FilePath defaultStackExecutable()
{
    // stack from brew or the installer script from https://docs.haskellstack.org
    // install to /usr/local/bin.
    if (HostOsInfo::isAnyUnixHost())
        return FilePath::fromString("/usr/local/bin/stack");
    return FilePath::fromString("stack");
}

FilePath HaskellManager::stackExecutable()
{
    return m_d->stackExecutable;
}

void HaskellManager::setStackExecutable(const FilePath &filePath)
{
    if (filePath == m_d->stackExecutable)
        return;
    m_d->stackExecutable = filePath;
    emit m_instance->stackExecutableChanged(m_d->stackExecutable);
}

void HaskellManager::openGhci(const FilePath &haskellFile)
{
    const QList<MimeType> mimeTypes = mimeTypesForFileName(haskellFile.toString());
    const bool isHaskell = Utils::anyOf(mimeTypes, [](const MimeType &mt) {
        return mt.inherits("text/x-haskell") || mt.inherits("text/x-literate-haskell");
    });
    const auto args = QStringList{"ghci"}
                      + (isHaskell ? QStringList{haskellFile.fileName()} : QStringList());
    Process p;
    p.setTerminalMode(TerminalMode::Detached);
    p.setCommand({stackExecutable(), args});
    p.setWorkingDirectory(haskellFile.absolutePath());
    p.start();
}

void HaskellManager::readSettings(QSettings *settings)
{
    m_d->stackExecutable = FilePath::fromString(
        settings->value(kStackExecutableKey, defaultStackExecutable().toString()).toString());
    emit m_instance->stackExecutableChanged(m_d->stackExecutable);
}

void HaskellManager::writeSettings(QSettings *settings)
{
    if (m_d->stackExecutable == defaultStackExecutable())
        settings->remove(kStackExecutableKey);
    else
        settings->setValue(kStackExecutableKey, m_d->stackExecutable.toString());
}

} // namespace Internal
} // namespace Haskell
