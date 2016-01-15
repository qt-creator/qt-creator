/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#ifndef CVSCLIENT_H
#define CVSCLIENT_H

#include "cvssettings.h"
#include <vcsbase/vcsbaseclient.h>

namespace Cvs {
namespace Internal {

class CvsSettings;

class CvsClient : public VcsBase::VcsBaseClient
{
    Q_OBJECT

public:
    CvsClient();

    CvsSettings &settings() const;
    void diff(const QString &workingDir, const QStringList &files,
              const QStringList &extraOptions = QStringList());
    QString findTopLevelForFile(const QFileInfo &file) const;
    QStringList revisionSpec(const QString &revision) const;
    StatusItem parseStatusLine(const QString &line) const;

protected:
    Utils::ExitCodeInterpreter *exitCodeInterpreter(VcsCommandTag cmd, QObject *parent) const;
    Core::Id vcsEditorKind(VcsCommandTag cmd) const;
};

} // namespace Internal
} // namespace Cvs

#endif // CVSCLIENT_H
