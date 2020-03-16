/****************************************************************************
**
** Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
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

#pragma once

#include "android_global.h"

#include "androidbuildapkstep.h"

#include <projectexplorer/buildstep.h>

QT_BEGIN_NAMESPACE
class QCheckBox;
class QComboBox;
class QLabel;
QT_END_NAMESPACE

namespace Utils {
class InfoLabel;
}

namespace Android {
namespace Internal {

class AndroidBuildApkWidget : public ProjectExplorer::BuildStepConfigWidget
{
    Q_OBJECT

public:
    explicit AndroidBuildApkWidget(AndroidBuildApkStep *step);

private:
    void setCertificates();
    void updateSigningWarning();
    void signPackageCheckBoxToggled(bool checked);
    void onOpenSslCheckBoxChanged();
    bool isOpenSslLibsIncluded();
    QString openSslIncludeFileContent(const Utils::FilePath &projectPath);

    QWidget *createApplicationGroup();
    QWidget *createSignPackageGroup();
    QWidget *createAdvancedGroup();
    QWidget *createCreateTemplatesGroup();
    QWidget *createAdditionalLibrariesGroup();

private:
    AndroidBuildApkStep *m_step = nullptr;
    QCheckBox *m_signPackageCheckBox = nullptr;
    Utils::InfoLabel *m_signingDebugWarningLabel = nullptr;
    QComboBox *m_certificatesAliasComboBox = nullptr;
    QCheckBox *m_addDebuggerCheckBox = nullptr;
    QCheckBox *m_openSslCheckBox = nullptr;
};

} // namespace Internal
} // namespace Android
