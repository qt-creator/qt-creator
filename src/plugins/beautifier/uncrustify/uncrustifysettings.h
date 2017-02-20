/****************************************************************************
**
** Copyright (C) 2016 Lorenz Haas
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

#include "../abstractsettings.h"
#include <utils/fileutils.h>
#include <QProcess>

namespace Beautifier {
namespace Internal {
namespace Uncrustify {

class UncrustifySettings : public AbstractSettings
{
    Q_OBJECT

public:
    UncrustifySettings();

    bool useOtherFiles() const;
    void setUseOtherFiles(bool useOtherFiles);

    bool useHomeFile() const;
    void setUseHomeFile(bool useHomeFile);

    bool useCustomStyle() const;
    void setUseCustomStyle(bool useCustomStyle);

    QString customStyle() const;
    void setCustomStyle(const QString &customStyle);

    bool formatEntireFileFallback() const;
    void setFormatEntireFileFallback(bool formatEntireFileFallback);

    QString documentationFilePath() const override;
    void createDocumentationFile() const override;

    void updateVersion() override;

    Utils::FileName specificConfigFile() const;
    void setSpecificConfigFile(const Utils::FileName &filePath);

    bool useSpecificConfigFile() const;
    void setUseSpecificConfigFile(bool useConfigFile);

private:
    QProcess m_versionProcess;
    void parseVersionProcessResult(int exitCode, QProcess::ExitStatus exitStatus);
};

} // namespace Uncrustify
} // namespace Internal
} // namespace Beautifier
