// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmt/infrastructure/uid.h"

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
    QString fileName() const { return m_fileName; }
    void setFileName(const QString &fileName);
    MPackage *rootPackage() const { return m_rootPackage; }
    void setRootPackage(MPackage *rootPackage);
    QString configPath() const { return m_configPath; }
    void setConfigPath(const QString &configPath);

private:
    Uid m_uid;
    QString m_fileName;
    MPackage *m_rootPackage = nullptr;
    QString m_configPath;
};

} // namespace qmt
