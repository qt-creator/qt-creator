/**************************************************************************
**
** Copyright (c) 2014 BogDan Vatra <bog_dan_ro@yahoo.com>
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef ANDROIDRUNCONFIGURATION_H
#define ANDROIDRUNCONFIGURATION_H

#include "androidconstants.h"
#include "androidconfigurations.h"

#include <projectexplorer/runconfiguration.h>

namespace QmakeProjectManager { class QmakeProFileNode; }

namespace Android {
namespace Internal {

class AndroidDeployStep;
class AndroidRunConfigurationFactory;

class AndroidRunConfiguration : public ProjectExplorer::RunConfiguration
{
    Q_OBJECT
    friend class AndroidRunConfigurationFactory;

public:
    AndroidRunConfiguration(ProjectExplorer::Target *parent, Core::Id id, const QString &path);

    QWidget *createConfigurationWidget();
    Utils::OutputFormatter *createOutputFormatter() const;

    void setArguments(const QString &args);
    QString proFilePath() const;

    const QString remoteChannel() const;

    bool isEnabled() const;
    QString disabledReason() const;
protected:
    AndroidRunConfiguration(ProjectExplorer::Target *parent, AndroidRunConfiguration *source);
    QString defaultDisplayName();

    bool fromMap(const QVariantMap &map);
    QVariantMap toMap() const;
private slots:
    void proFileUpdated(QmakeProjectManager::QmakeProFileNode *pro, bool success, bool parseInProgress);
private:
    void init();

    QString m_proFilePath;
    bool m_parseSuccess;
    bool m_parseInProgress;
};

} // namespace Internal
} // namespace Android

#endif // ANDROIDRUNCONFIGURATION_H
