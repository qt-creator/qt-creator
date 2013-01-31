/**************************************************************************
**
** Copyright (c) 2013 BogDan Vatra <bog_dan_ro@yahoo.com>
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

#ifndef ANDROIDSETTINGSWIDGET_H
#define ANDROIDSETTINGSWIDGET_H

#include "androidconfigurations.h"

#include <QList>
#include <QString>
#include <QWidget>
#include <QAbstractTableModel>

QT_BEGIN_NAMESPACE
class Ui_AndroidSettingsWidget;
QT_END_NAMESPACE

namespace Android {
namespace Internal {

class AvdModel: public QAbstractTableModel
{
    Q_OBJECT
public:
    void setAvdList(const QVector<AndroidDeviceInfo> &list);
    QString avdName(const QModelIndex &index);

protected:
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;

private:
    QVector<AndroidDeviceInfo> m_list;
};

class AndroidSettingsWidget : public QWidget
{
    Q_OBJECT
public:
    // Todo: This would be so much simpler if it just used Utils::PathChooser!!!
    AndroidSettingsWidget(QWidget *parent);
    ~AndroidSettingsWidget();

    void saveSettings(bool saveNow = false);
    QString searchKeywords() const;

private slots:
    void sdkLocationEditingFinished();
    void ndkLocationEditingFinished();
    void antLocationEditingFinished();
    void gdbLocationEditingFinished();
    void gdbserverLocationEditingFinished();
    void gdbLocationX86EditingFinished();
    void gdbserverLocationX86EditingFinished();
    void openJDKLocationEditingFinished();
    void browseSDKLocation();
    void browseNDKLocation();
    void browseAntLocation();
    void browseGdbLocation();
    void browseGdbserverLocation();
    void browseGdbLocationX86();
    void browseGdbserverLocationX86();
    void browseOpenJDKLocation();
    void toolchainVersionIndexChanged(QString);
    void addAVD();
    void removeAVD();
    void startAVD();
    void avdActivated(QModelIndex);
    void dataPartitionSizeEditingFinished();
    void manageAVD();

private:
    void initGui();
    bool checkSDK(const Utils::FileName &location);
    bool checkNDK(const Utils::FileName &location);
    void fillToolchainVersions();

    Ui_AndroidSettingsWidget *m_ui;
    AndroidConfig m_androidConfig;
    AvdModel m_AVDModel;
    bool m_saveSettingsRequested;
};

} // namespace Internal
} // namespace Android

#endif // ANDROIDSETTINGSWIDGET_H
