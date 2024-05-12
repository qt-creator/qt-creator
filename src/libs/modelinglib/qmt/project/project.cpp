// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "project.h"

using Utils::FilePath;

namespace qmt {

Project::Project()
{
}

Project::~Project()
{
}

void Project::setUid(const Uid &uid)
{
    m_uid = uid;
}

bool Project::hasFileName() const
{
    return !m_fileName.isEmpty();
}

void Project::setFileName(const FilePath &fileName)
{
    m_fileName = fileName;
}

void Project::setRootPackage(MPackage *rootPackage)
{
    m_rootPackage = rootPackage;
}

void Project::setConfigPath(const FilePath &configPath)
{
    m_configPath = configPath;
}

} // namespace qmt
