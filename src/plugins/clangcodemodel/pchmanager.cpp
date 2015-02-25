/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "pchmanager.h"
#include "utils.h"
#include "clangutils.h"

#include <coreplugin/icore.h>
#include <coreplugin/progressmanager/progressmanager.h>

#include <utils/qtcassert.h>
#include <utils/runextensions.h>

#include <QFile>

using namespace ClangCodeModel;
using namespace ClangCodeModel::Internal;
using namespace CPlusPlus;

PchManager *PchManager::m_instance = 0;

PchManager::PchManager(QObject *parent)
    : QObject(parent)
{
    Q_ASSERT(!m_instance);
    m_instance = this;

    QObject *msgMgr = Core::MessageManager::instance();
    connect(this, SIGNAL(pchMessage(QString,Core::MessageManager::PrintToOutputPaneFlags)),
            msgMgr, SLOT(write(QString,Core::MessageManager::PrintToOutputPaneFlags)));

    connect(&m_pchGenerationWatcher, SIGNAL(finished()),
            this, SLOT(updateActivePchFiles()));
}

PchManager::~PchManager()
{
    Q_ASSERT(m_instance);
    m_instance = 0;
    qDeleteAll(m_projectSettings.values());
    m_projectSettings.clear();
}

PchManager *PchManager::instance()
{
    return m_instance;
}

PchInfo::Ptr PchManager::pchInfo(const ProjectPart::Ptr &projectPart) const
{
    QMutexLocker locker(&m_mutex);

    return m_activePchFiles[projectPart];
}

ClangProjectSettings *PchManager::settingsForProject(ProjectExplorer::Project *project)
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

void PchManager::setPCHInfo(const QList<ProjectPart::Ptr> &projectParts,
                            const PchInfo::Ptr &pchInfo,
                            const QPair<bool, QStringList> &msgs)
{
    QMutexLocker locker(&m_mutex);

    foreach (ProjectPart::Ptr pPart, projectParts)
        m_activePchFiles[pPart] = pchInfo;

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
            emit pchMessage(msgs.second.join(QLatin1Char('\n')), Core::MessageManager::Flash);
    }
}

void PchManager::clangProjectSettingsChanged()
{
    ClangProjectSettings *cps = qobject_cast<ClangProjectSettings *>(sender());
    if (!cps)
        return;

    onProjectPartsUpdated(cps->project());
}

void PchManager::onAboutToRemoveProject(ProjectExplorer::Project *project)
{
    Q_UNUSED(project);

    // we cannot ask the ModelManager for the parts, because, depending on
    // the order of signal delivery, it might already have wiped any information
    // about the project.

    updateActivePchFiles();
}

void PchManager::onProjectPartsUpdated(ProjectExplorer::Project *project)
{
    ClangProjectSettings *cps = settingsForProject(project);
    Q_ASSERT(cps);

    CppTools::CppModelManager *mmi = CppTools::CppModelManager::instance();
    const QList<ProjectPart::Ptr> projectParts = mmi->projectInfo(
                cps->project()).projectParts();
    updatePchInfo(cps, projectParts);
}

void PchManager::updatePchInfo(ClangProjectSettings *cps,
                               const QList<ProjectPart::Ptr> &projectParts)
{
    if (m_pchGenerationWatcher.isRunning()) {
        m_pchGenerationWatcher.waitForFinished();
    }

    const QString customPchFile = cps->customPchFile();
    const ClangProjectSettings::PchUsage pchUsage = cps->pchUsage();

    void (*updateFunction)(QFutureInterface<void> &future,
                           const PchManager::UpdateParams params) = 0;
    QString message;
    if (pchUsage == ClangProjectSettings::PchUse_None
            || (pchUsage == ClangProjectSettings::PchUse_Custom && customPchFile.isEmpty())) {
        updateFunction = &PchManager::doPchInfoUpdateNone;
        message = QLatin1String("updatePchInfo: switching to none");
    } else if (pchUsage == ClangProjectSettings::PchUse_BuildSystem_Fuzzy) {
        updateFunction = &PchManager::doPchInfoUpdateFuzzy;
        message = QLatin1String("updatePchInfo: switching to build system (fuzzy)");
    } else if (pchUsage == ClangProjectSettings::PchUse_BuildSystem_Exact) {
        updateFunction = &PchManager::doPchInfoUpdateExact;
        message = QLatin1String("updatePchInfo: switching to build system (exact)");
    } else if (pchUsage == ClangProjectSettings::PchUse_Custom) {
        updateFunction = &PchManager::doPchInfoUpdateCustom;
        message = QLatin1String("updatePchInfo: switching to custom") + customPchFile;
    }

    QTC_ASSERT(updateFunction && !message.isEmpty(), return);

    Core::MessageManager::write(message, Core::MessageManager::Silent);
    QFuture<void> future = QtConcurrent::run(updateFunction,
                                             UpdateParams(customPchFile, projectParts));
    m_pchGenerationWatcher.setFuture(future);
    Core::ProgressManager::addTask(future, tr("Precompiling"), "Key.Tmp.Precompiling");
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

void PchManager::doPchInfoUpdateNone(QFutureInterface<void> &future,
                                     const PchManager::UpdateParams params)
{
    future.setProgressRange(0, 1);
    PchInfo::Ptr emptyPch = PchInfo::createEmpty();
    PchManager::instance()->setPCHInfo(params.projectParts, emptyPch,
                                       qMakePair(true, QStringList()));
    future.setProgressValue(1);
}

void PchManager::doPchInfoUpdateFuzzy(QFutureInterface<void> &future,
                                      const PchManager::UpdateParams params)
{
    typedef ProjectPart::HeaderPath HeaderPath;
    QHash<QString, QSet<HeaderPath>> headers;
    QHash<QString, QSet<QByteArray> > definesPerPCH;
    QHash<QString, bool> objc;
    QHash<QString, bool> cplusplus;
    QHash<QString, ProjectPart::QtVersion> qtVersions;
    QHash<QString, ProjectPart::LanguageVersion> languageVersions;
    QHash<QString, ProjectPart::LanguageExtensions> languageExtensionsMap;
    QHash<QString, QList<ProjectPart::Ptr> > inputToParts;
    foreach (const ProjectPart::Ptr &projectPart, params.projectParts) {
        if (projectPart->precompiledHeaders.isEmpty())
            continue;
        const QString &pch = projectPart->precompiledHeaders.first(); // TODO: support more than 1 PCH file.
        if (!QFile(pch).exists())
            continue;
        inputToParts[pch].append(projectPart);

        headers[pch].unite(QSet<HeaderPath>::fromList(projectPart->headerPaths));
        languageVersions[pch] = std::max(languageVersions.value(pch, ProjectPart::C89),
                                         projectPart->languageVersion);
        languageExtensionsMap[pch] = languageExtensionsMap[pch] | projectPart->languageExtensions;

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
        projectPart->languageVersion = languageVersions[pch];
        projectPart->languageExtensions = languageExtensionsMap[pch];
        projectPart->headerPaths = headers[pch].toList();
        projectPart->updateLanguageFeatures();

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
        projectPart.clear();

        PchManager *pchManager = PchManager::instance();
        PchInfo::Ptr pchInfo = pchManager->findMatchingPCH(pch, options, true);
        QPair<bool, QStringList> msgs = qMakePair(true, QStringList());
        if (pchInfo.isNull()) {

            pchInfo = PchInfo::createWithFileName(pch, options, objc[pch]);
            msgs = precompile(pchInfo);
        }
        pchManager->setPCHInfo(inputToParts[pch], pchInfo, msgs);
        future.setProgressValue(future.progressValue() + 1);
    }

    future.setProgressValue(future.progressValue() + 1);
}

void PchManager::doPchInfoUpdateExact(QFutureInterface<void> &future,
                                      const PchManager::UpdateParams params)
{
    future.setProgressRange(0, params.projectParts.size() + 1);
    future.setProgressValue(0);
    foreach (const ProjectPart::Ptr &projectPart, params.projectParts) {
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

        PchManager *pchManager = PchManager::instance();
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

    future.setProgressValue(future.progressValue() + 1);
}

void PchManager::doPchInfoUpdateCustom(QFutureInterface<void> &future,
                                       const PchManager::UpdateParams params)
{
    future.setProgressRange(0, 1);
    future.setProgressValue(0);

    ProjectPart::HeaderPaths headers;
    bool objc = false;
    bool cplusplus = false;
    ProjectPart::Ptr united(new ProjectPart());
    united->languageVersion = ProjectPart::C89;
    foreach (const ProjectPart::Ptr &projectPart, params.projectParts) {
        headers += projectPart->headerPaths;
        united->languageVersion = std::max(united->languageVersion, projectPart->languageVersion);
        united->qtVersion = std::max(united->qtVersion, projectPart->qtVersion);
        objc |= hasObjCFiles(projectPart);
        cplusplus |= hasCppFiles(projectPart);
    }
    united->updateLanguageFeatures();
    united->headerPaths = headers;
    QStringList opts = Utils::createClangOptions(
                united, getPrefixFileKind(objc, cplusplus));
    united.clear();

    PchManager *pchManager = PchManager::instance();
    PchInfo::Ptr pchInfo = pchManager->findMatchingPCH(params.customPchFile, opts, true);
    QPair<bool, QStringList> msgs = qMakePair(true, QStringList());;
    if (future.isCanceled())
        return;
    if (pchInfo.isNull()) {
        pchInfo = PchInfo::createWithFileName(params.customPchFile, opts, objc);
        msgs = precompile(pchInfo);
    }
    pchManager->setPCHInfo(params.projectParts, pchInfo, msgs);
    future.setProgressValue(1);
}

PchInfo::Ptr PchManager::findMatchingPCH(const QString &inputFileName,
                                         const QStringList &options,
                                         bool fuzzyMatching) const
{
    QMutexLocker locker(&m_mutex);

    if (fuzzyMatching) {
        QStringList opts = options;
        opts.sort();
        foreach (PchInfo::Ptr pchInfo, m_activePchFiles.values()) {
            if (pchInfo->inputFileName() != inputFileName)
                continue;
            QStringList pchOpts = pchInfo->options();
            pchOpts.sort();
            if (pchOpts == opts)
                return pchInfo;
        }
    } else {
        foreach (PchInfo::Ptr pchInfo, m_activePchFiles.values())
            if (pchInfo->inputFileName() == inputFileName
                    && pchInfo->options() == options)
                return pchInfo;
    }

    return PchInfo::Ptr();
}

void PchManager::updateActivePchFiles()
{
    QMutexLocker locker(&m_mutex);

    QSet<ProjectPart::Ptr> activeParts;
    CppTools::CppModelManager *mmi = CppTools::CppModelManager::instance();
    foreach (const CppTools::ProjectInfo &pi, mmi->projectInfos())
        activeParts.unite(QSet<ProjectPart::Ptr>::fromList(pi.projectParts()));
    QList<ProjectPart::Ptr> partsWithPCHFiles = m_activePchFiles.keys();
    foreach (ProjectPart::Ptr pPart, partsWithPCHFiles)
        if (!activeParts.contains(pPart))
            m_activePchFiles.remove(pPart);
}
