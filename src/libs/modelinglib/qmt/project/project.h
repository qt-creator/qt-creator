/****************************************************************************
**
** Copyright (C) 2016 Jochen Becher
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
