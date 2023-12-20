// Copyright (C) 2019 Luxoft Sweden AB
// Copyright (C) 2018 Pelagicore AG
// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "appmanagertargetinformation.h"

#include "appmanagerconstants.h"

#include <projectexplorer/kitaspects.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/runcontrol.h>
#include <projectexplorer/target.h>

#include <utils/qtcassert.h>

#include <qmakeprojectmanager/qmakeproject.h>

#include <qtsupport/profilereader.h>

#include <yaml-cpp/yaml.h>

using namespace ProjectExplorer;
using namespace QmakeProjectManager;
using namespace QtSupport;
using namespace Utils;

namespace AppManager {
namespace Internal {

static QString trimmedAndUnquoted(const QByteArray &value)
{
    auto result = QString::fromUtf8(value.trimmed());
    if (result.length() > 2
            && result.startsWith('\'')
            && result.endsWith('\'')) {
        result.remove(result.length() - 1, 1);
        result.remove(0, 1);
    }
    return result;
}

Manifest::Manifest(const QString &fileNameIn)
{
    QFile file(fileNameIn);
    if (file.exists() && file.open(QIODevice::ReadOnly)) {
        fileName = fileNameIn;
        // TODO: make reading valid and optimized
        const auto lines = file.readAll().split('\n');
        for (const auto& line: lines) {
            if (line.trimmed().isEmpty()) {
                continue;
            }
            if (line.trimmed() == "---") {
                continue;
            }
            const auto parts = line.split(':');
            if (parts.size() != 2)
                continue;
            if (parts.first().trimmed() == "formatVersion") {
                formatVersion = parts.last().toInt();
                continue;
            }
            if (parts.first().trimmed() == "formatType") {
                formatType = trimmedAndUnquoted(parts.last());
                continue;
            }
            if (parts.first().trimmed() == "id") {
                id = trimmedAndUnquoted(parts.last());
                continue;
            }
            if (parts.first().trimmed() == "icon") {
                icon = trimmedAndUnquoted(parts.last());
                continue;
            }
            if (parts.first().trimmed() == "code") {
                code = trimmedAndUnquoted(parts.last());
                continue;
            }
            if (parts.first().trimmed() == "runtime") {
                runtime = trimmedAndUnquoted(parts.last());
                continue;
            }
        }
        file.close();
    }
}

Manifest::Manifest(const QmakeProjectManager::InstallsList &installs)
{
    for (const auto &item : installs.items) {
        if (item.active) {
            for (const auto &file : item.files) {
                const QFileInfo fileInfo(file.fileName);
                if (fileInfo.fileName().toLower() == "info.yaml") {
                    Manifest result(fileInfo.absoluteFilePath());
                    if (!result.fileName.isEmpty()) {
                        result.installPathSuffix = installs.targetPath;
                        *this = result;
                        return;
                    }
                }
            }
        }
    }
}

QString readProVariable(QmakeBuildSystem *bs, QmakeProFile *file, const QString &variableName)
{
    QString result;
    if (!file)
        return result;
    auto project = file->project();
    if (!project)
        return result;
    if (variableName.isEmpty())
        return result;
    if (auto proFileReader = bs->createProFileReader(file)) {
        const FilePath filePath = file->filePath();
        const QString deviceRoot = filePath.withNewPath("/").toFSPathString();
        if (auto proFile = proFileReader->parsedProFile(deviceRoot, filePath.path())) {
            if (proFileReader->accept(proFile, QMakeEvaluator::LoadAll)) {
                result = proFileReader->value(variableName);
            }
        }
        bs->destroyProFileReader(proFileReader);
    }
    return result;
}

FilePath destinationDirectoryFor(const QmakeProjectManager::TargetInformation &ti)
{
    if (ti.destDir.isEmpty())
        return ti.buildDir;
    if (QDir::isRelativePath(ti.destDir.toString()))
        return FilePath::fromString(QDir::cleanPath(ti.buildDir.toString() + '/' + ti.destDir.toString()));
    return ti.destDir;
}

QString packageFileNameFor(const QmakeProFile *file, const QString &baseName)
{
    if (!file)
        return QString();
    if (baseName.isEmpty())
        return QString();

    QmakeProjectManager::TargetInformation ti = file->targetInformation();
    if (!ti.valid)
        return QString();

    return QDir(destinationDirectoryFor(ti).toString()).absoluteFilePath(baseName + ".pkg");
}

QList<TargetInformation> TargetInformation::readFromProject(const Target *target , const QString &buildKey)
{
    QList<TargetInformation> result;
    if (!target->project()->rootProjectNode())
        return result;
    if (auto bs = qobject_cast<QmakeBuildSystem*>(target->buildSystem())) {
        const QmakeProFile *const file = bs->rootProFile();
        if (!file || file->parseInProgress())
            return result;
        const auto rootInstallPathSuffix = readProVariable(bs, bs->rootProFile(), Constants::QMAKE_AM_PACKAGE_DIR_VARIABLE);
        target->project()->rootProjectNode()->forEachProjectNode([&](const ProjectNode *pn) {
            auto node = dynamic_cast<const QmakeProFileNode *>(pn);
            if (!node || !node->includedInExactParse())
                return;
            if (!buildKey.isEmpty() && buildKey != node->filePath().toString())
                return;
            QmakeProjectManager::TargetInformation ti = node->targetInformation();
            if (!ti.valid)
                return;
            TargetInformation ati;
            const auto manifestFileName = readProVariable(bs, node->proFile(), Constants::QMAKE_AM_MANIFEST_VARIABLE);
            if (!manifestFileName.isEmpty()) {
                ati.manifest = manifestFileName;
                if (ati.manifest.fileName.isEmpty()) {
                    ati.manifest = node->filePath().toFileInfo().dir().absoluteFilePath(manifestFileName);
                }
            }
            if (ati.manifest.fileName.isEmpty()) {
                ati.manifest = node->proFile()->installsList();
            }
            if (ati.manifest.fileName.isEmpty())
                return;
            QString destDir = ti.destDir.toString();
            QString workingDir;
            if (!destDir.isEmpty()) {
                bool workingDirIsBaseDir = false;
                if (destDir == ti.buildTarget)
                    workingDirIsBaseDir = true;
                if (QDir::isRelativePath(destDir))
                    destDir = QDir::cleanPath(ti.buildDir.toString() + '/' + destDir);

                if (workingDirIsBaseDir)
                    workingDir = ti.buildDir.toString();
                else
                    workingDir = destDir;
            } else {
                workingDir = ti.buildDir.toString();
            }
            ati.buildDirectory = QDir(workingDir);
            ati.projectFile = node->filePath().toFileInfo();
            ati.packageFile = QFileInfo(packageFileNameFor(node->proFile(), ati.manifest.id));
            ati.buildKey = ati.projectFile.absoluteFilePath();
            ati.displayName = ati.packageFile.fileName();
            const FilePath relativePathInProject = node->filePath().relativeChildPath(bs->projectDirectory());
            if (!relativePathInProject.isEmpty()) {
                ati.displayNameUniquifier = QString::fromLatin1(" (%1)").arg(relativePathInProject.toUserOutput());
            }
            auto installPathSuffix = readProVariable(bs, node->proFile(), Constants::QMAKE_AM_PACKAGE_DIR_VARIABLE);
            if (installPathSuffix.isEmpty())
                installPathSuffix = rootInstallPathSuffix;
            if (!installPathSuffix.isEmpty()) {
                ati.manifest.installPathSuffix = installPathSuffix;
                while (installPathSuffix.startsWith('/'))
                    installPathSuffix.remove(0, 1);
                ati.packageSourcesDirectory = QDir(QString("%1/%2").arg(ati.buildDirectory.absolutePath(), installPathSuffix));
            } else {
                ati.packageSourcesDirectory = ati.buildDirectory;
            }
            result.append(ati);
        });
    } else {
        QVariantList packageTargets = target->project()->extraData(AppManager::Constants::APPMAN_PACKAGE_TARGETS).toList();
//        qDebug() << "APPMAN TARGETS" << packageTargets;

        for (const auto &packageTarget : packageTargets) {
            const QVariantMap packageTargetMap = packageTarget.toMap();
            const FilePath manifestFilePath = packageTargetMap.value("manifestFilePath").value<FilePath>();
            const QString cmakeTarget = packageTargetMap.value("cmakeTarget").toString();
            const FilePath packageFilePath = packageTargetMap.value("packageFilePath").value<FilePath>();
            const bool isBuiltinPackage = packageTargetMap.value("isBuiltinPackage").toBool();

            const Utils::expected_str<QByteArray> localFileContents = manifestFilePath.fileContents();
            if (!localFileContents.has_value()) {
                qWarning() << "NOPE:" << localFileContents.error();
            }

            auto createTargetInformation = [buildKey, manifestFilePath, cmakeTarget, packageFilePath, isBuiltinPackage, &result](const YAML::Node &document) {
                const QString id = QString::fromStdString(document["id"].as<std::string>());
                const QString runtime = QString::fromStdString(document["runtime"].as<std::string>());

                if (!buildKey.isEmpty() && buildKey != id)
                    return;

                TargetInformation ati;
                ati.displayNameUniquifier = id + " App";
                ati.displayName = id + " App";
                ati.buildKey = id;
                ati.manifest.fileName = manifestFilePath.path();
                ati.manifest.id = id;
                ati.manifest.runtime = runtime;
                ati.isBuiltin = isBuiltinPackage;
                ati.cmakeBuildTarget = cmakeTarget;
                ati.packageFile = QFileInfo(packageFilePath.path());

//                qCritical() << "CREATE CONFIG: BUILTIN:" << isBuiltinPackage << id << "TARGET:" << cmakeTarget;

                result.append(ati);
            };

            try {
                std::vector<YAML::Node> documents = YAML::LoadAll(*localFileContents);
                if (documents.size() != 2)
                    throw std::runtime_error("Must contain two documents");
                YAML::Node header = documents[0];
                YAML::Node document = documents[1];

                std::string formatType = header["formatType"].as<std::string>();
                if (formatType == "am-package") {
                    for (const auto &applicationNode : document["applications"])
                        createTargetInformation(applicationNode);
                } else if (formatType == "am-application") {
                    createTargetInformation(document);
                } else {
                    throw std::runtime_error("unknown formatType");
                }

            } catch (const std::exception &e) {
                qWarning() << "NOPE:" << e.what();
            }
        }
    }
    return result;
}

TargetInformation::TargetInformation(const Target *target)
{
    if (!target)
        return;
    if (target->buildSystem()->isParsing())
        return;
    auto project = target->project();
    if (!project)
        return;

    const RunConfiguration *rc = target->activeRunConfiguration();
    if (!rc)
        return;
    if (rc->id() != Constants::RUNCONFIGURATION_ID)
        return;

    const auto buildKey = rc->buildKey();
    if (buildKey.isEmpty())
        return;

    const auto targetInfoList = TargetInformation::readFromProject(target, buildKey);
    if (targetInfoList.isEmpty())
        return;

    *this = targetInfoList.first();

    device = DeviceKitAspect::device(target->kit());
    remote = device && device->type() != ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE;
    runDirectory = remote ? QDir(Constants::REMOTE_DEFAULT_TMP_PATH) : buildDirectory;
}

bool TargetInformation::isValid() const
{
    static const QFileInfo INVALID_FILE_INFO = QFileInfo();
    return !manifest.fileName.isEmpty() && packageFile != INVALID_FILE_INFO;
}

FilePath TargetInformation::workingDirectory() const
{
    return FilePath::fromString(runDirectory.absolutePath());
}

} // namespace Internal
} // namespace AppManager
