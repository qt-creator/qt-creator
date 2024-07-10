// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmt/infrastructure/uid.h"

#include <utils/filepath.h>

#include <QString>

namespace qmt {

class MPackage;

class QMT_EXPORT Project
{
public:
    Project();
    ~Project();

    Uid uid() const { return m_uid; }
    void setUid(const Uid &uid);
    bool hasFileName() const;
    Utils::FilePath fileName() const { return m_fileName; }
    void setFileName(const Utils::FilePath &fileName);
    MPackage *rootPackage() const { return m_rootPackage; }
    void setRootPackage(MPackage *rootPackage);
    Utils::FilePath configPath() const { return m_configPath; }
    void setConfigPath(const Utils::FilePath &configPath);

private:
    Uid m_uid;
    Utils::FilePath m_fileName;
    MPackage *m_rootPackage = nullptr;
    Utils::FilePath m_configPath;
};

} // namespace qmt
