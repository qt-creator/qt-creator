/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef TARGETSETUPPAGE_H
#define TARGETSETUPPAGE_H

#include "qtversionmanager.h"

#include <QtCore/QList>
#include <QtCore/QPair>
#include <QtCore/QSet>
#include <QtCore/QString>

#include <QtGui/QIcon>
#include <QtGui/QWizard>

QT_BEGIN_NAMESPACE
class QLabel;
class QMenu;
class QPushButton;
class QTreeWidget;
class QTreeWidgetItem;
QT_END_NAMESPACE

namespace Qt4ProjectManager {
class Qt4Project;

namespace Internal {

namespace Ui {
class TargetSetupPage;
}

class TargetSetupPage : public QWizardPage
{
    Q_OBJECT

public:
    struct ImportInfo {
        ImportInfo() :
            version(0),
            isTemporary(false),
            buildConfig(QtVersion::QmakeBuildConfig(0)),
            isExistingBuild(false),
            isShadowBuild(false)
        {
            if (version && version->isValid())
                buildConfig = version->defaultBuildConfig();
        }

        ImportInfo(const ImportInfo &other) :
            version(other.version),
            isTemporary(other.isTemporary),
            buildConfig(other.buildConfig),
            additionalArguments(other.additionalArguments),
            directory(other.directory),
            isExistingBuild(other.isExistingBuild),
            isShadowBuild(other.isShadowBuild)
        { }

        QtVersion *version;
        bool isTemporary;
        QtVersion::QmakeBuildConfigs buildConfig;
        QString additionalArguments;
        QString directory;
        bool isExistingBuild;
        bool isShadowBuild;
    };

    explicit TargetSetupPage(QWidget* parent = 0);
    ~TargetSetupPage();

    void initializePage();

    void setImportInfos(const QList<ImportInfo> &infos);
    QList<ImportInfo> importInfos() const;

    void setImportDirectoryBrowsingEnabled(bool browsing);
    void setImportDirectoryBrowsingLocation(const QString &directory);
    void setPreferMobile(bool mobile);

    static QList<ImportInfo> importInfosForKnownQtVersions();
    static QList<ImportInfo> filterImportInfos(const QSet<QString> &validTargets,
                                               const QList<ImportInfo> &infos);

    static QList<ImportInfo> scanDefaultProjectDirectories(Qt4Project *project);
    static QList<ImportInfo> recursivelyCheckDirectoryForBuild(const QString &directory,
                                                               const QString &proFile, int maxdepth = 3);

    bool hasSelection() const;
    bool isTargetSelected(const QString &targetid) const;
    bool isComplete() const;

    bool setupProject(Qt4Project *project);

public slots:
    void setProFilePath(const QString &dir);
    void checkAll(bool checked);
    void checkOne(bool, QTreeWidgetItem *);

private slots:
    void itemWasChanged();
    void checkAllButtonClicked();
    void checkAllTriggered();
    void uncheckAllTriggered();
    void checkOneTriggered();
    void addShadowBuildLocation();
    void handleDoubleClicks(QTreeWidgetItem *, int);
    void contextMenuRequested(const QPoint & pos);

private:
    void resetInfos();
    QPair<QIcon, QString> reportIssues(QtVersion *version, const QString &buildDir);
    void updateVersionItem(QTreeWidgetItem *, int);

    QList<ImportInfo> m_infos;
    bool m_preferMobile;
    bool m_toggleWillCheck;
    QString m_proFilePath;
    QString m_defaultShadowBuildLocation;

    Ui::TargetSetupPage *m_ui;

    QMenu *m_contextMenu;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // TARGETSETUPPAGE_H
