/****************************************************************************
**
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

#ifndef QTOPTIONSPAGE_H
#define QTOPTIONSPAGE_H

#include "debugginghelperbuildtask.h"
#include <coreplugin/dialogs/ioptionspage.h>

#include <QIcon>
#include <QPointer>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QSortFilterProxyModel;
class QTextBrowser;
class QUrl;
QT_END_NAMESPACE

namespace ProjectExplorer { class ToolChain; }
namespace Utils {
class TreeModel;
class TreeItem;
}

namespace QtSupport {

class BaseQtVersion;
class QtConfigWidget;

namespace Internal {
class QtVersionItem;

namespace Ui {
class QtVersionManager;
class QtVersionInfo;
class DebuggingHelper;
}

class QtOptionsPageWidget : public QWidget
{
    Q_OBJECT

public:
    QtOptionsPageWidget(QWidget *parent = 0);
    ~QtOptionsPageWidget();
    QList<BaseQtVersion *> versions() const;
    void apply();

private:
    void updateDescriptionLabel();
    void userChangedCurrentVersion();
    void updateWidgets();
    void updateDebuggingHelperUi();
    BaseQtVersion *currentVersion() const;
    QtVersionItem *currentItem() const;
    void showDebuggingBuildLog(const QtVersionItem *item);

    const QString m_specifyNameString;

    Internal::Ui::QtVersionManager *m_ui;
    Internal::Ui::QtVersionInfo *m_versionUi;
    Internal::Ui::DebuggingHelper *m_debuggingHelperUi;
    QTextBrowser *m_infoBrowser;
    int m_defaultVersion;
    QIcon m_invalidVersionIcon;
    QIcon m_warningVersionIcon;
    QIcon m_validVersionIcon;
    QtConfigWidget *m_configurationWidget;

private slots:
    void updateQtVersions(const QList<int> &, const QList<int> &, const QList<int> &);
    void qtVersionChanged();
    void versionChanged(const QModelIndex &current, const QModelIndex &previous);
    void addQtDir();
    void removeQtDir();
    void editPath();
    void updateCleanUpButton();
    void updateCurrentQtName();
    void buildDebuggingHelper(DebuggingHelperBuildTask::Tools tools
                              = DebuggingHelperBuildTask::AllTools);
    void buildQmlDump();
    void slotShowDebuggingBuildLog();
    void debuggingHelperBuildFinished(int qtVersionId, const QString &output,
                                      DebuggingHelperBuildTask::Tools tools);
    void cleanUpQtVersions();
    void toolChainsUpdated();
    void selectedToolChainChanged(int index);

    void qtVersionsDumpUpdated(const Utils::FileName &qmakeCommand);
    void setInfoWidgetVisibility();
    void infoAnchorClicked(const QUrl &);

private:
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

    Utils::TreeModel *m_model;
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


#endif // QTOPTIONSPAGE_H
