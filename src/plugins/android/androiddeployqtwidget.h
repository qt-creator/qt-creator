/**************************************************************************
**
** Copyright (c) 2014 BogDan Vatra <bog_dan_ro@yahoo.com>
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
#ifndef ANDROIDDEPLOYQTWIDGET_H
#define ANDROIDDEPLOYQTWIDGET_H

#include <projectexplorer/buildstep.h>

#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui { class AndroidDeployQtWidget; }
QT_END_NAMESPACE

namespace QmakeProjectManager { class QmakeBuildConfiguration; }

namespace Android {
namespace Internal {
class AndroidDeployQtStep;
class AndroidExtraLibraryListModel;
class AndroidDeployQtWidget : public ProjectExplorer::BuildStepConfigWidget
{
    Q_OBJECT

public:
    AndroidDeployQtWidget(AndroidDeployQtStep *step);
    ~AndroidDeployQtWidget();

private slots:
    void setTargetSdk(const QString &sdk);
    void setMinistro();
    void setDeployLocalQtLibs();
    void setBundleQtLibs();
    void installMinistro();
    void cleanLibsOnDevice();
    void resetDefaultDevices();
    void createKeyStore();
    void certificatesAliasComboBoxCurrentIndexChanged(const QString &alias);
    void certificatesAliasComboBoxActivated(const QString &alias);
    void activeBuildConfigurationChanged();
    void updateSigningWarning();
    void openPackageLocationCheckBoxToggled(bool checked);
    void verboseOutputCheckBoxToggled(bool checked);
    void updateKeyStorePath(const QString &path);
    void signPackageCheckBoxToggled(bool checked);
    void updateInputFileUi();
    void inputFileComboBoxIndexChanged();
    void createManifestButton();
    void addAndroidExtraLib();
    void removeAndroidExtraLib();
    void checkEnableRemoveButton();
    void checkProjectTemplate();

private:
    virtual QString summaryText() const;
    virtual QString displayName() const;
    void setCertificates();

    Ui::AndroidDeployQtWidget *m_ui;
    AndroidDeployQtStep *m_step;
    AndroidExtraLibraryListModel *m_extraLibraryListModel;
    QmakeProjectManager::QmakeBuildConfiguration *m_currentBuildConfiguration;
    bool m_ignoreChange;
};

}
}
#endif // ANDROIDDEPLOYQTWIDGET_H
