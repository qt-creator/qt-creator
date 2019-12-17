/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "ssh_global.h"

#include <utils/fileutils.h>

#include <functional>

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

namespace QSsh {

class QSSH_EXPORT SshSettings
{
public:
    static void loadSettings(QSettings *settings);
    static void storeSettings(QSettings *settings);

    static void setConnectionSharingEnabled(bool share);
    static bool connectionSharingEnabled();

    static void setConnectionSharingTimeout(int timeInMinutes);
    static int connectionSharingTimeout();

    static void setSshFilePath(const Utils::FilePath &ssh);
    static Utils::FilePath sshFilePath();

    static void setSftpFilePath(const Utils::FilePath &sftp);
    static Utils::FilePath sftpFilePath();

    static void setAskpassFilePath(const Utils::FilePath &askPass);
    static Utils::FilePath askpassFilePath();

    static void setKeygenFilePath(const Utils::FilePath &keygen);
    static Utils::FilePath keygenFilePath();

    using SearchPathRetriever = std::function<Utils::FilePaths()>;
    static void setExtraSearchPathRetriever(const SearchPathRetriever &pathRetriever);
};

} // namespace QSsh
