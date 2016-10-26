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

#pragma once

#include <coreplugin/dialogs/ioptionspage.h>

#include <utils/fileutils.h>
#include <utils/treemodel.h>

#include <QIcon>
#include <QPointer>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QSortFilterProxyModel;
class QTextBrowser;
class QUrl;
QT_END_NAMESPACE

namespace ProjectExplorer { class ToolChain; }

namespace QtSupport {

class BaseQtVersion;
class QtConfigWidget;

namespace Internal {
class QtVersionItem;

namespace Ui {
class QtVersionManager;
class QtVersionInfo;
}

class QtOptionsPageWidget : public QWidget
{
    Q_OBJECT

public:
    QtOptionsPageWidget(QWidget *parent = 0);
    ~QtOptionsPageWidget();
    void apply();

private:
    void updateDescriptionLabel();
    void userChangedCurrentVersion();
    void updateWidgets();
    BaseQtVersion *currentVersion() const;
    QtVersionItem *currentItem() const;
    void showDebuggingBuildLog(const QtVersionItem *item);

    const QString m_specifyNameString;

    Internal::Ui::QtVersionManager *m_ui;
    Internal::Ui::QtVersionInfo *m_versionUi;
    QTextBrowser *m_infoBrowser;
    int m_defaultVersion;
    QIcon m_invalidVersionIcon;
    QIcon m_warningVersionIcon;
    QIcon m_validVersionIcon;
    QtConfigWidget *m_configurationWidget;

private:
    void updateQtVersions(const QList<int> &, const QList<int> &, const QList<int> &);
    void versionChanged(const QModelIndex &current, const QModelIndex &previous);
    void addQtDir();
    void removeQtDir();
    void editPath();
    void updateCleanUpButton();
    void updateCurrentQtName();

    void cleanUpQtVersions();
    void toolChainsUpdated();

    void qtVersionsDumpUpdated(const Utils::FileName &qmakeCommand);
    void setInfoWidgetVisibility();
    void infoAnchorClicked(const QUrl &);

    struct ValidityInfo {
        QString description;
        QString message;
        QString toolTip;
        QIcon icon;
    };
    ValidityInfo validInformation(const BaseQtVersion *version);
    QList<ProjectExplorer::ToolChain*> toolChains(const BaseQtVersion *version);
    QByteArray defaultToolChainId(const BaseQtVersion *version);

    bool isNameUnique(const BaseQtVersion *version);
    void updateVersionItem(QtVersionItem *item);

    Utils::TreeModel<Utils::TreeItem, Utils::TreeItem, QtVersionItem> *m_model;
    QSortFilterProxyModel *m_filterModel;
    Utils::TreeItem *m_autoItem;
    Utils::TreeItem *m_manualItem;
};

class QtOptionsPage : public Core::IOptionsPage
{
    Q_OBJECT

public:
    QtOptionsPage();

    QWidget *widget();
    void apply();
    void finish();

private:
    QPointer<QtOptionsPageWidget> m_widget;
};

} //namespace Internal
} //namespace QtSupport
