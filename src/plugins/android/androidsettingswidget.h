/****************************************************************************
**
** Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
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

#include "androidconfigurations.h"

#include <QList>
#include <QString>
#include <QWidget>
#include <QAbstractTableModel>
#include <QFutureWatcher>

#include <memory>

QT_BEGIN_NAMESPACE
class Ui_AndroidSettingsWidget;
QT_END_NAMESPACE

namespace Android {
namespace Internal {

class AndroidAvdManager;

class AvdModel: public QAbstractTableModel
{
    Q_OBJECT
public:
    void setAvdList(const AndroidDeviceInfoList &list);
    QString avdName(const QModelIndex &index) const;
    QModelIndex indexForAvdName(const QString &avdName) const;

protected:
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;

private:
    AndroidDeviceInfoList m_list;
};

class AndroidSettingsWidget : public QWidget
{
    Q_OBJECT
public:
    // Todo: This would be so much simpler if it just used Utils::PathChooser!!!
    AndroidSettingsWidget(QWidget *parent = 0);
    ~AndroidSettingsWidget();

    void saveSettings();

private:
    void sdkLocationEditingFinished();
    void ndkLocationEditingFinished();
    void searchForAnt(const Utils::FileName &location);
    void antLocationEditingFinished();
    void openJDKLocationEditingFinished();
    void openSDKDownloadUrl();
    void openNDKDownloadUrl();
    void openAntDownloadUrl();
    void openOpenJDKDownloadUrl();
    void addAVD();
    void avdAdded();
    void removeAVD();
    void startAVD();
    void avdActivated(const QModelIndex &);
    void dataPartitionSizeEditingFinished();
    void manageAVD();
    void createKitToggled();
    void useGradleToggled();

    void checkGdbFinished();
    void showGdbWarningDialog();
    void updateAvds();
    void updateGradleBuildUi();

private:
    enum Mode { Sdk = 1, Ndk = 2, Java = 4, All = Sdk | Ndk | Java };
    enum State { NotSet = 0, Okay = 1, Error = 2 };
    void check(Mode mode);
    void applyToUi(Mode mode);
    bool sdkLocationIsValid() const;
    bool sdkPlatformToolsInstalled() const;
    void startUpdateAvd();
    void enableAvdControls();
    void disableAvdControls();

    State m_sdkState;
    State m_ndkState;
    QString m_ndkErrorMessage;
    int m_ndkCompilerCount;
    QString m_ndkMissingQtArchs;
    State m_javaState;

    Ui_AndroidSettingsWidget *m_ui;
    AndroidConfig m_androidConfig;
    AvdModel m_AVDModel;
    QFutureWatcher<AndroidConfig::CreateAvdInfo> m_futureWatcher;
    QFutureWatcher<QPair<QStringList, bool>> m_checkGdbWatcher;
    QStringList m_gdbCheckPaths;

    QFutureWatcher<AndroidDeviceInfoList> m_virtualDevicesWatcher;
    QString m_lastAddedAvd;
    std::unique_ptr<AndroidAvdManager> m_avdManager;
};

} // namespace Internal
} // namespace Android
