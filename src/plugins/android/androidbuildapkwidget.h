/**************************************************************************
**
** Copyright (C) 2015 BogDan Vatra <bog_dan_ro@yahoo.com>
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
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
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef ANDROIDBUILDAPKWIDGET_H
#define ANDROIDBUILDAPKWIDGET_H

#include "android_global.h"

#include <projectexplorer/buildstep.h>

#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui { class AndroidBuildApkWidget; }
QT_END_NAMESPACE

namespace QmakeProjectManager { class QmakeBuildConfiguration; }

namespace Android {
class AndroidBuildApkStep;

class ANDROID_EXPORT AndroidBuildApkWidget : public ProjectExplorer::BuildStepConfigWidget
{
    Q_OBJECT

public:
    AndroidBuildApkWidget(AndroidBuildApkStep *step);
    ~AndroidBuildApkWidget();

private slots:
    void setTargetSdk(const QString &sdk);
    void setMinistro();
    void setDeployLocalQtLibs();
    void setBundleQtLibs();
    void createKeyStore();
    void certificatesAliasComboBoxCurrentIndexChanged(const QString &alias);
    void certificatesAliasComboBoxActivated(const QString &alias);
    void updateSigningWarning();
    void updateDebugDeploySigningWarning();
    void useGradleCheckBoxToggled(bool checked);
    void openPackageLocationCheckBoxToggled(bool checked);
    void verboseOutputCheckBoxToggled(bool checked);
    void updateKeyStorePath(const QString &path);
    void signPackageCheckBoxToggled(bool checked);

private:
    virtual QString summaryText() const;
    virtual QString displayName() const;
    void setCertificates();

    Ui::AndroidBuildApkWidget *m_ui;
    AndroidBuildApkStep *m_step;
};

}

#endif // ANDROIDBUILDAPKWIDGET_H
