/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include "haskellmanager.h"

#include <coreplugin/messagemanager.h>
#include <utils/algorithm.h>
#include <utils/commandline.h>
#include <utils/hostosinfo.h>
#include <utils/mimeutils.h>
#include <utils/processenums.h>
#include <utils/qtcprocess.h>

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
    auto p = new QtcProcess(m_instance);
    p->setTerminalMode(TerminalMode::On);
    p->setCommand({stackExecutable(), args});
    p->setWorkingDirectory(haskellFile.absolutePath());
    connect(p, &QtcProcess::done, p, [p] {
        if (p->result() != ProcessResult::FinishedWithSuccess) {
            Core::MessageManager::writeDisrupting(
                tr("Failed to run GHCi: \"%1\".").arg(p->errorString()));
        }
        p->deleteLater();
    });
    p->start();
}

void HaskellManager::readSettings(QSettings *settings)
{
    m_d->stackExecutable = FilePath::fromString(
                settings->value(kStackExecutableKey,
                                defaultStackExecutable().toString()).toString());
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
