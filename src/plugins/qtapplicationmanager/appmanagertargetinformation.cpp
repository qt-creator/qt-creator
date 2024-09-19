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
#include <projectexplorer/taskhub.h>

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

QList<TargetInformation> TargetInformation::readFromProject(const Target *target , const QString &buildKey)
{
    QList<TargetInformation> result;
    if (!target->project()->rootProjectNode())
        return result;

    QVariantList packageTargets = target->project()->extraData(AppManager::Constants::APPMAN_PACKAGE_TARGETS).toList();
//        qDebug() << "APPMAN TARGETS" << packageTargets;

    for (const auto &packageTarget : packageTargets) {
        const QVariantMap packageTargetMap = packageTarget.toMap();
        const FilePath manifestFilePath = packageTargetMap.value("manifestFilePath").value<FilePath>();
        const QString cmakeTarget = packageTargetMap.value("cmakeTarget").toString();
        const FilePath packageFilePath = packageTargetMap.value("packageFilePath").value<FilePath>();
        const bool isBuiltinPackage = packageTargetMap.value("isBuiltinPackage").toBool();

        auto createTargetInformation = [buildKey, manifestFilePath, cmakeTarget, packageFilePath, isBuiltinPackage, &result](const YAML::Node &document) {
            const QString id = QString::fromStdString(document["id"].as<std::string>());
            const QString runtime = QString::fromStdString(document["runtime"].as<std::string>());
            const QString code = QString::fromStdString(document["code"].as<std::string>());

            if (!buildKey.isEmpty() && buildKey != id)
                return;

            TargetInformation ati;
            ati.displayNameUniquifier = id + " [App]";
            ati.displayName = id + " [App]";
            ati.buildKey = id;
            ati.manifest.filePath = manifestFilePath;
            ati.manifest.id = id;
            ati.manifest.runtime = runtime;
            ati.manifest.code = code;
            ati.isBuiltin = isBuiltinPackage;
            ati.cmakeBuildTarget = cmakeTarget;
            ati.packageFilePath = packageFilePath;

//                qCritical() << "CREATE CONFIG: BUILTIN:" << isBuiltinPackage << id << "TARGET:" << cmakeTarget;

            result.append(ati);
        };

        try {
            const Utils::expected_str<QByteArray> localFileContents = manifestFilePath.fileContents();
            if (!localFileContents.has_value())
                throw std::runtime_error("Invalid empty file");

            std::vector<YAML::Node> documents = YAML::LoadAll(*localFileContents);
            if (documents.size() != 2)
                throw std::runtime_error("Must contain exactly two documents");
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
            const QString error = QString("Error parsing package manifest: %1").arg(QString::fromUtf8(e.what()));
            ProjectExplorer::TaskHub::addTask(BuildSystemTask(ProjectExplorer::Task::Error, error, manifestFilePath));
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
    if (rc->id() != Constants::RUNCONFIGURATION_ID &&
        rc->id() != Constants::RUNANDDEBUGCONFIGURATION_ID)
        return;

    const auto buildKey = rc->buildKey();
    if (buildKey.isEmpty())
        return;

    const auto targetInfoList = TargetInformation::readFromProject(target, buildKey);
    if (targetInfoList.isEmpty())
        return;

    *this = targetInfoList.first();

    runDirectory = Constants::REMOTE_DEFAULT_TMP_PATH;
}

bool TargetInformation::isValid() const
{
    return !manifest.filePath.isEmpty() && manifest.filePath.isFile();
}

} // namespace Internal
} // namespace AppManager
