// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "mcuqmlprojectnode.h"
#include "mcusupport_global.h"
#include "mcusupportplugin.h"

#include <utils/filepath.h>
#include <utils/osspecificaspects.h>
#include <utils/process.h>

#include <projectexplorer/project.h>
#include <projectexplorer/projectnodes.h>

#include <QHostInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QObject>
#include <QProcess>
#include <QRegularExpression>

#include <memory>

namespace McuSupport::Internal {

using namespace ProjectExplorer;
using namespace Utils;

class McuQmlProjectFolderNode : public FolderNode
{
public:
    explicit McuQmlProjectFolderNode(const FilePath &path)
        : FolderNode(path)
    {}
    bool showInSimpleTree() const override { return true; }
};

class McuQmlProjectNode : public FolderNode
{
public:
    explicit McuQmlProjectNode(const FilePath &projectFolder, const FilePath &inputsJsonFile);
    bool showInSimpleTree() const override { return true; }
    bool populateModuleNode(FolderNode *moduleNode, const QVariantMap &moduleObject);
};
}; // namespace McuSupport::Internal
