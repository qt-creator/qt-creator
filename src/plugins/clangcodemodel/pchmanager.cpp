/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "pchmanager.h"
#include "utils.h"
#include "clangutils.h"

#include <coreplugin/icore.h>
#include <coreplugin/progressmanager/progressmanager.h>

#include <utils/runextensions.h>

#include <QFile>

using namespace ClangCodeModel;
using namespace ClangCodeModel::Internal;
using namespace CPlusPlus;

PCHManager *PCHManager::m_instance = 0;

PCHManager::PCHManager(QObject *parent)
    : QObject(parent)
{
    Q_ASSERT(!m_instance);
    m_instance = this;

    QObject *msgMgr = Core::MessageManager::instance();
    connect(this, SIGNAL(pchMessage(QString, Core::MessageManager::PrintToOutputPaneFlags)),
            msgMgr, SLOT(write(QString, Core::MessageManager::PrintToOutputPaneFlags)));

    connect(&m_pchGenerationWatcher, SIGNAL(finished()),
            this, SLOT(updateActivePCHFiles()));
}

PCHManager::~PCHManager()
{
    Q_ASSERT(m_instance);
    m_instance = 0;
    qDeleteAll(m_projectSettings.values());
    m_projectSettings.clear();
}

PCHManager *PCHManager::instance()
{
    return m_instance;
}

PchInfo::Ptr PCHManager::pchInfo(const ProjectPart::Ptr &projectPart) const
{
    QMutexLocker locker(&m_mutex);

    return m_activePCHFiles[projectPart];
}

ClangProjectSettings *PCHManager::settingsForProject(ProjectExplorer::Project *project)
{
    QMutexLocker locker(&m_mutex);

    ClangProjectSettings *cps = m_projectSettings.value(project);
    if (!cps) {
        cps = new ClangProjectSettings(project);
        m_projectSettings.insert(project, cps);
        cps->pullSettings();
        connect(cps, SIGNAL(pchSettingsChanged()),
                this, SLOT(clangProjectSettingsChanged()));
    }
    return cps;
}

void PCHManager::setPCHInfo(const QList<ProjectPart::Ptr> &projectParts,
                            const PchInfo::Ptr &pchInfo,
                            const QPair<bool, QStringList> &msgs)
{
    QMutexLocker locker(&m_mutex);

    foreach (ProjectPart::Ptr pPart, projectParts)
        m_activePCHFiles[pPart] = pchInfo;

    if (pchInfo) {
        if (msgs.first) {
            if (!pchInfo->fileName().isEmpty())
                emit pchMessage(tr("Successfully generated PCH file \"%1\".").arg(
                                    pchInfo->fileName()), Core::MessageManager::Silent);
        } else {
            emit pchMessage(tr("Failed to generate PCH file \"%1\".").arg(
                                pchInfo->fileName()), Core::MessageManager::Silent);
        }
        if (!msgs.second.isEmpty())
            emit pchMessage(msgs.second.join(QLatin1String("\n")), Core::MessageManager::Flash);
    }
}

void PCHManager::clangProjectSettingsChanged()
{
    ClangProjectSettings *cps = qobject_cast<ClangProjectSettings *>(sender());
    if (!cps)
        return;

    onProjectPartsUpdated(cps->project());
}

void PCHManager::onAboutToRemoveProject(ProjectExplorer::Project *project)
{
    Q_UNUSED(project);

    // we cannot ask the ModelManager for the parts, because, depending on
    // the order of signal delivery, it might already have wiped any information
    // about the project.

    updateActivePCHFiles();
}

void PCHManager::onProjectPartsUpdated(ProjectExplorer::Project *project)
{
    ClangProjectSettings *cps = settingsForProject(project);
    Q_ASSERT(cps);

    CppTools::CppModelManagerInterface *mmi = CppTools::CppModelManagerInterface::instance();
    const QList<ProjectPart::Ptr> projectParts = mmi->projectInfo(
                cps->project()).projectParts();
    updatePchInfo(cps, projectParts);

    emit pchInfoUpdated();
}

void PCHManager::updatePchInfo(ClangProjectSettings *cps,
                               const QList<ProjectPart::Ptr> &projectParts)
{
    if (m_pchGenerationWatcher.isRunning()) {
//        m_pchGenerationWatcher.cancel();
        m_pchGenerationWatcher.waitForFinished();
    }

    QFuture<void> future = QtConcurrent::run(&PCHManager::doPchInfoUpdate,
                                             cps->pchUsage(),
                                             cps->customPchFile(),
                                             projectParts);
    m_pchGenerationWatcher.setFuture(future);
    Core::ProgressManager::addTask(future, tr("Precompiling..."), "Key.Tmp.Precompiling");
}

namespace {

bool hasObjCFiles(const CppTools::ProjectPart::Ptr &projectPart)
{
    foreach (const CppTools::ProjectFile &file, projectPart->files) {
        switch (file.kind) {
        case CppTools::ProjectFile::ObjCHeader:
        case CppTools::ProjectFile::ObjCSource:
        case CppTools::ProjectFile::ObjCXXHeader:
        case CppTools::ProjectFile::ObjCXXSource:
            return true;
        default:
            break;
        }
    }
    return false;
}

bool hasCppFiles(const CppTools::ProjectPart::Ptr &projectPart)
{
    foreach (const CppTools::ProjectFile &file, projectPart->files) {
        switch (file.kind) {
        case CppTools::ProjectFile::CudaSource:
        case CppTools::ProjectFile::CXXHeader:
        case CppTools::ProjectFile::CXXSource:
        case CppTools::ProjectFile::OpenCLSource:
        case CppTools::ProjectFile::ObjCXXHeader:
        case CppTools::ProjectFile::ObjCXXSource:
            return true;
        default:
            break;
        }
    }
    return false;
}

CppTools::ProjectFile::Kind getPrefixFileKind(bool hasObjectiveC, bool hasCPlusPlus)
{
    if (hasObjectiveC && hasCPlusPlus)
        return CppTools::ProjectFile::ObjCXXHeader;
    else if (hasObjectiveC)
        return CppTools::ProjectFile::ObjCHeader;
    else if (hasCPlusPlus)
        return CppTools::ProjectFile::CXXHeader;
    return CppTools::ProjectFile::CHeader;
}

}

void PCHManager::doPchInfoUpdate(QFutureInterface<void> &future,
                                 ClangProjectSettings::PchUsage pchUsage,
                                 const QString customPchFile,
                                 const QList<ProjectPart::Ptr> projectParts)
{
    PCHManager *pchManager = PCHManager::instance();

//    qDebug() << "switching to" << pchUsage;

    if (pchUsage == ClangProjectSettings::PchUse_None
            || (pchUsage == ClangProjectSettings::PchUse_Custom && customPchFile.isEmpty())) {
        future.setProgressRange(0, 2);
        Core::MessageManager::write(QLatin1String("updatePchInfo: switching to none"),
                                    Core::MessageManager::Silent);
        PchInfo::Ptr emptyPch = PchInfo::createEmpty();
        pchManager->setPCHInfo(projectParts, emptyPch, qMakePair(true, QStringList()));
        future.setProgressValue(1);
    } else if (pchUsage == ClangProjectSettings::PchUse_BuildSystem_Fuzzy) {
        Core::MessageManager::write(
                    QLatin1String("updatePchInfo: switching to build system (fuzzy)"),
                    Core::MessageManager::Silent);
        QHash<QString, QSet<QString> > includes, frameworks;
        QHash<QString, QSet<QByteArray> > definesPerPCH;
        QHash<QString, bool> objc;
        QHash<QString, bool> cplusplus;
        QHash<QString, ProjectPart::QtVersion> qtVersions;
        QHash<QString, ProjectPart::CVersion> cVersions;
        QHash<QString, ProjectPart::CXXVersion> cxxVersions;
        QHash<QString, ProjectPart::CXXExtensions> cxxExtensionsMap;
        QHash<QString, QList<ProjectPart::Ptr> > inputToParts;
        foreach (const ProjectPart::Ptr &projectPart, projectParts) {
            if (projectPart->precompiledHeaders.isEmpty())
                continue;
            const QString &pch = projectPart->precompiledHeaders.first(); // TODO: support more than 1 PCH file.
            if (!QFile(pch).exists())
                continue;
            inputToParts[pch].append(projectPart);

            includes[pch].unite(QSet<QString>::fromList(projectPart->includePaths));
            frameworks[pch].unite(QSet<QString>::fromList(projectPart->frameworkPaths));
            cVersions[pch] = std::max(cVersions.value(pch, ProjectPart::C89), projectPart->cVersion);
            cxxVersions[pch] = std::max(cxxVersions.value(pch, ProjectPart::CXX98), projectPart->cxxVersion);
            cxxExtensionsMap[pch] = cxxExtensionsMap[pch] | projectPart->cxxExtensions;

            if (hasObjCFiles(projectPart))
                objc[pch] = true;
            if (hasCppFiles(projectPart))
                cplusplus[pch] = true;

            QSet<QByteArray> projectDefines = QSet<QByteArray>::fromList(projectPart->toolchainDefines.split('\n'));
            QMutableSetIterator<QByteArray> iter(projectDefines);
            while (iter.hasNext()){
                QByteArray v = iter.next();
                if (v.startsWith("#define _") || v.isEmpty()) // TODO: see ProjectPart::createClangOptions
                    iter.remove();
            }
            projectDefines.unite(QSet<QByteArray>::fromList(projectPart->projectDefines.split('\n')));

            if (definesPerPCH.contains(pch)) {
                definesPerPCH[pch].intersect(projectDefines);
            } else {
                definesPerPCH[pch] = projectDefines;
            }

            qtVersions[pch] = projectPart->qtVersion;
        }

        future.setProgressRange(0, definesPerPCH.size() + 1);
        future.setProgressValue(0);

        foreach (const QString &pch, inputToParts.keys()) {
            if (future.isCanceled())
                return;
            ProjectPart::Ptr projectPart(new ProjectPart);
            projectPart->qtVersion = qtVersions[pch];
            projectPart->cVersion = cVersions[pch];
            projectPart->cxxVersion = cxxVersions[pch];
            projectPart->cxxExtensions = cxxExtensionsMap[pch];
            projectPart->includePaths = includes[pch].toList();
            projectPart->frameworkPaths = frameworks[pch].toList();

            QList<QByteArray> defines = definesPerPCH[pch].toList();
            if (!defines.isEmpty()) {
                projectPart->projectDefines = defines[0];
                for (int i = 1; i < defines.size(); ++i) {
                    projectPart->projectDefines += '\n';
                    projectPart->projectDefines += defines[i];
                }
            }

            CppTools::ProjectFile::Kind prefixFileKind =
                    getPrefixFileKind(objc.value(pch, false), cplusplus.value(pch, false));

            QStringList options = Utils::createClangOptions(projectPart, prefixFileKind);
            projectPart.reset();

            PchInfo::Ptr pchInfo = pchManager->findMatchingPCH(pch, options, true);
            QPair<bool, QStringList> msgs = qMakePair(true, QStringList());
            if (pchInfo.isNull()) {

                pchInfo = PchInfo::createWithFileName(pch, options, objc[pch]);
                msgs = precompile(pchInfo);
            }
            pchManager->setPCHInfo(inputToParts[pch], pchInfo, msgs);
            future.setProgressValue(future.progressValue() + 1);
        }
    } else if (pchUsage == ClangProjectSettings::PchUse_BuildSystem_Exact) {
        future.setProgressRange(0, projectParts.size() + 1);
        future.setProgressValue(0);
        Core::MessageManager::write(
                    QLatin1String("updatePchInfo: switching to build system (exact)"),
                    Core::MessageManager::Silent);
        foreach (const ProjectPart::Ptr &projectPart, projectParts) {
            if (future.isCanceled())
                return;
            if (projectPart->precompiledHeaders.isEmpty())
                continue;
            const QString &pch = projectPart->precompiledHeaders.first(); // TODO: support more than 1 PCH file.
            if (!QFile(pch).exists())
                continue;

            const bool hasObjC = hasObjCFiles(projectPart);
            QStringList options = Utils::createClangOptions(
                        projectPart, getPrefixFileKind(hasObjC, hasCppFiles(projectPart)));

            PchInfo::Ptr pchInfo = pchManager->findMatchingPCH(pch, options, false);
            QPair<bool, QStringList> msgs = qMakePair(true, QStringList());
            if (pchInfo.isNull()) {
                pchInfo = PchInfo::createWithFileName(pch, options, hasObjC);
                msgs = precompile(pchInfo);
            }
            pchManager->setPCHInfo(QList<ProjectPart::Ptr>() << projectPart,
                                   pchInfo, msgs);
            future.setProgressValue(future.progressValue() + 1);
        }
    } else if (pchUsage == ClangProjectSettings::PchUse_Custom) {
        future.setProgressRange(0, 2);
        future.setProgressValue(0);
        Core::MessageManager::write(
                    QLatin1String("updatePchInfo: switching to custom") + customPchFile,
                    Core::MessageManager::Silent);

        QSet<QString> includes, frameworks;
        bool objc = false;
        bool cplusplus = false;
        ProjectPart::Ptr united(new ProjectPart());
        united->cxxVersion = ProjectPart::CXX98;
        foreach (const ProjectPart::Ptr &projectPart, projectParts) {
            includes.unite(QSet<QString>::fromList(projectPart->includePaths));
            frameworks.unite(QSet<QString>::fromList(projectPart->frameworkPaths));
            united->cVersion = std::max(united->cVersion, projectPart->cVersion);
            united->cxxVersion = std::max(united->cxxVersion, projectPart->cxxVersion);
            united->qtVersion = std::max(united->qtVersion, projectPart->qtVersion);
            objc |= hasObjCFiles(projectPart);
            cplusplus |= hasCppFiles(projectPart);
        }
        united->frameworkPaths = frameworks.toList();
        united->includePaths = includes.toList();
        QStringList opts = Utils::createClangOptions(
                    united, getPrefixFileKind(objc, cplusplus));
        united.clear();

        PchInfo::Ptr pchInfo = pchManager->findMatchingPCH(customPchFile, opts, true);
        QPair<bool, QStringList> msgs = qMakePair(true, QStringList());;
        if (future.isCanceled())
            return;
        if (pchInfo.isNull()) {
            pchInfo = PchInfo::createWithFileName(customPchFile, opts, objc);
            msgs = precompile(pchInfo);
        }
        pchManager->setPCHInfo(projectParts, pchInfo, msgs);
        future.setProgressValue(1);
    }

    future.setProgressValue(future.progressValue() + 1);
}

PchInfo::Ptr PCHManager::findMatchingPCH(const QString &inputFileName,
                                         const QStringList &options,
                                         bool fuzzyMatching) const
{
    QMutexLocker locker(&m_mutex);

    if (fuzzyMatching) {
        QStringList opts = options;
        opts.sort();
        foreach (PchInfo::Ptr pchInfo, m_activePCHFiles.values()) {
            if (pchInfo->inputFileName() != inputFileName)
                continue;
            QStringList pchOpts = pchInfo->options();
            pchOpts.sort();
            if (pchOpts == opts)
                return pchInfo;
        }
    } else {
        foreach (PchInfo::Ptr pchInfo, m_activePCHFiles.values())
            if (pchInfo->inputFileName() == inputFileName
                    && pchInfo->options() == options)
                return pchInfo;
    }

    return PchInfo::Ptr();
}

void PCHManager::updateActivePCHFiles()
{
    QMutexLocker locker(&m_mutex);

    QSet<ProjectPart::Ptr> activeParts;
    CppTools::CppModelManagerInterface *mmi = CppTools::CppModelManagerInterface::instance();
    foreach (const CppTools::CppModelManagerInterface::ProjectInfo &pi, mmi->projectInfos())
        activeParts.unite(QSet<ProjectPart::Ptr>::fromList(pi.projectParts()));
    QList<ProjectPart::Ptr> partsWithPCHFiles = m_activePCHFiles.keys();
    foreach (ProjectPart::Ptr pPart, partsWithPCHFiles)
        if (!activeParts.contains(pPart))
            m_activePCHFiles.remove(pPart);
}
