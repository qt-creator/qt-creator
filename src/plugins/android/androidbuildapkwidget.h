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
#include "androidextralibrarylistmodel.h"

#include <projectexplorer/buildstep.h>

#include <QListView>
#include <QToolButton>

QT_BEGIN_NAMESPACE
namespace Ui { class AndroidBuildApkWidget; }
QT_END_NAMESPACE

namespace Android {
namespace Internal {

class AndroidBuildApkInnerWidget : public ProjectExplorer::BuildStepConfigWidget
{
    Q_OBJECT

public:
    AndroidBuildApkInnerWidget(AndroidBuildApkStep *step);
    ~AndroidBuildApkInnerWidget() override;

private:
    void setTargetSdk(const QString &sdk);
    void createKeyStore();
    void certificatesAliasComboBoxCurrentIndexChanged(const QString &alias);
    void certificatesAliasComboBoxActivated(const QString &alias);
    void updateSigningWarning();
    void openPackageLocationCheckBoxToggled(bool checked);
    void verboseOutputCheckBoxToggled(bool checked);
    void updateKeyStorePath(const QString &path);
    void signPackageCheckBoxToggled(bool checked);

    void setCertificates();

    Ui::AndroidBuildApkWidget *m_ui;
    AndroidBuildApkStep *m_step;
};

class AndroidBuildApkWidget : public ProjectExplorer::BuildStepConfigWidget
{
    Q_OBJECT

public:
    explicit AndroidBuildApkWidget(AndroidBuildApkStep *step);

signals:
    void requestAndroidTemplates();

private:
    void addAndroidExtraLib();
    void removeAndroidExtraLib();
    void checkEnableRemoveButton();

private:
    QListView *m_androidExtraLibsListView = nullptr;
    QToolButton *m_removeAndroidExtraLibButton = nullptr;

    AndroidBuildApkStep *m_step = nullptr;
    Android::AndroidExtraLibraryListModel *m_extraLibraryListModel = nullptr;
    bool m_ignoreChange = false;
};

} // namespace Internal
} // namespace Android
