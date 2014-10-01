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

#ifndef ANDROIDPACKAGECREATIONWIDGET_H
#define ANDROIDPACKAGECREATIONWIDGET_H

#include <projectexplorer/buildstep.h>

#include <QAbstractListModel>
#include <QStringList>

QT_BEGIN_NAMESPACE
class QFileSystemWatcher;

namespace Ui { class AndroidPackageCreationWidget; }
QT_END_NAMESPACE

namespace QmakeProjectManager { class QmakeBuildConfiguration; }

namespace Android {
namespace Internal {
class AndroidPackageCreationStep;

class CheckModel: public QAbstractListModel
{
    Q_OBJECT
public:
    CheckModel(QObject *parent = 0);
    void setAvailableItems(const QStringList &items);
    void setCheckedItems(const QStringList &items);
    const QStringList &checkedItems();
    QVariant data(const QModelIndex &index, int role) const;
    void moveUp(int index);
    int rowCount(const QModelIndex &parent = QModelIndex()) const;

protected:
    bool setData(const QModelIndex &index, const QVariant &value, int role);
    Qt::ItemFlags flags(const QModelIndex &index) const;
private:
    QStringList m_availableItems;
    QStringList m_checkedItems;
};

class AndroidPackageCreationWidget : public ProjectExplorer::BuildStepConfigWidget
{
    Q_OBJECT
public:
    AndroidPackageCreationWidget(AndroidPackageCreationStep *step);

    virtual QString summaryText() const;
    virtual QString displayName() const;

public slots:
    void readElfInfo();

private:
    void setEnabledSaveDiscardButtons(bool enabled);
    void setCertificates();

private slots:
    void initGui();
    void updateAndroidProjectInfo();

    void setTargetSDK(const QString &sdk);

    void setQtLibs(QModelIndex, QModelIndex);
    void setPrebundledLibs(QModelIndex, QModelIndex);
    void prebundledLibSelected(const QModelIndex &index);
    void prebundledLibMoveUp();
    void prebundledLibMoveDown();

    void updateRequiredLibrariesModels();
    void on_signPackageCheckBox_toggled(bool checked);
    void on_KeystoreCreatePushButton_clicked();
    void on_KeystoreLocationPushButton_clicked();
    void on_certificatesAliasComboBox_activated(const QString &alias);
    void on_certificatesAliasComboBox_currentIndexChanged(const QString &alias);

    void on_openPackageLocationCheckBox_toggled(bool checked);

    void updateSigningWarning();
    void activeBuildConfigurationChanged();
private:
    AndroidPackageCreationStep *const m_step;
    Ui::AndroidPackageCreationWidget *const m_ui;
    CheckModel *m_qtLibsModel;
    CheckModel *m_prebundledLibs;
    QFileSystemWatcher *m_fileSystemWatcher;
    QmakeProjectManager::QmakeBuildConfiguration *m_currentBuildConfiguration;
};

} // namespace Internal
} // namespace Android

#endif // ANDROIDPACKAGECREATIONWIDGET_H
