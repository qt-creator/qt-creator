/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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
    CvsClient(CvsSettings *settings);

    CvsSettings *settings() const;
    void diff(const QString &workingDir, const QStringList &files,
              const QStringList &extraOptions = QStringList());
    QString findTopLevelForFile(const QFileInfo &file) const;
    QStringList revisionSpec(const QString &revision) const;
    StatusItem parseStatusLine(const QString &line) const;


protected:
    Utils::ExitCodeInterpreter *exitCodeInterpreter(VcsCommandTag cmd, QObject *parent) const;
    Core::Id vcsEditorKind(VcsCommandTag cmd) const;
    VcsBase::VcsBaseEditorParameterWidget *createDiffEditor(const QString &workingDir,
                                                            const QStringList &files,
                                                            const QStringList &extraOptions);
private:
};

} // namespace Internal
} // namespace Cvs

#endif // CVSCLIENT_H
