/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#ifndef QTOPTIONSPAGE_H
#define QTOPTIONSPAGE_H

#include "debugginghelperbuildtask.h"
#include <coreplugin/dialogs/ioptionspage.h>

#include <QWidget>
#include <QIcon>

QT_BEGIN_NAMESPACE
class QTreeWidgetItem;
class QTextBrowser;
class QUrl;
QT_END_NAMESPACE

namespace ProjectExplorer {
class ToolChain;
}

namespace QtSupport {

class BaseQtVersion;
class QtConfigWidget;

namespace Internal {
namespace Ui {
class QtVersionManager;
class QtVersionInfo;
class DebuggingHelper;
}

class QtOptionsPageWidget : public QWidget
{
    Q_OBJECT

public:
    QtOptionsPageWidget(QWidget *parent);
    ~QtOptionsPageWidget();
    QList<BaseQtVersion *> versions() const;
    void finish();
    void apply();
    QString searchKeywords() const;

private:
    void updateDescriptionLabel();
    void userChangedCurrentVersion();
    void updateWidgets();
    void updateDebuggingHelperUi();
    void fixQtVersionName(int index);
    int indexForTreeItem(const QTreeWidgetItem *item) const;
    QTreeWidgetItem *treeItemForIndex(int index) const;
    BaseQtVersion *currentVersion() const;
    int currentIndex() const;
    void showDebuggingBuildLog(const QTreeWidgetItem *currentItem);

    const QString m_specifyNameString;

    Internal::Ui::QtVersionManager *m_ui;
    Internal::Ui::QtVersionInfo *m_versionUi;
    Internal::Ui::DebuggingHelper *m_debuggingHelperUi;
    QTextBrowser *m_infoBrowser;
    QList<BaseQtVersion *> m_versions;
    int m_defaultVersion;
    QIcon m_invalidVersionIcon;
    QIcon m_warningVersionIcon;
    QIcon m_validVersionIcon;
    QtConfigWidget *m_configurationWidget;

private slots:
    void updateQtVersions(const QList<int> &, const QList<int> &, const QList<int> &);
    void qtVersionChanged();
    void versionChanged(QTreeWidgetItem *item, QTreeWidgetItem *old);
    void addQtDir();
    void removeQtDir();
    void editPath();
    void updateCleanUpButton();
    void updateCurrentQtName();
    void buildDebuggingHelper(DebuggingHelperBuildTask::Tools tools
                              = DebuggingHelperBuildTask::AllTools);
    void buildGdbHelper();
    void buildQmlDump();
    void buildQmlDebuggingLibrary();
    void buildQmlObserver();
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
    QString defaultToolChainId(const BaseQtVersion *verison);

    QTreeWidgetItem *m_autoItem;
    QTreeWidgetItem *m_manualItem;
};

class QtOptionsPage : public Core::IOptionsPage
{
    Q_OBJECT

public:
    QtOptionsPage();

    QWidget *createPage(QWidget *parent);
    void apply();
    void finish() {}
    bool matches(const QString &) const;

private:
    QtOptionsPageWidget *m_widget;
    QString m_searchKeywords;
};

} //namespace Internal
} //namespace QtSupport


#endif // QTOPTIONSPAGE_H
