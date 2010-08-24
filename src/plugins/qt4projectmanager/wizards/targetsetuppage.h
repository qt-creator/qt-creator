/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
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
class QPushButton;
class QTreeWidget;
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
            isExistingBuild(false)
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
            isExistingBuild(other.isExistingBuild)
        { }

        QtVersion *version;
        bool isTemporary;
        QtVersion::QmakeBuildConfigs buildConfig;
        QStringList additionalArguments;
        QString directory;
        bool isExistingBuild;
    };

    explicit TargetSetupPage(QWidget* parent = 0);
    ~TargetSetupPage();

    void setImportInfos(const QList<ImportInfo> &infos);
    QList<ImportInfo> importInfos() const;

    void setImportDirectoryBrowsingEnabled(bool browsing);
    void setImportDirectoryBrowsingLocation(const QString &directory);
    void setShowLocationInformation(bool location);
    void setPreferMobile(bool mobile);

    static QList<ImportInfo> importInfosForKnownQtVersions();
    static QList<ImportInfo> filterImportInfos(const QSet<QString> &validTargets,
                                               const QList<ImportInfo> &infos);

    static QList<ImportInfo> recursivelyCheckDirectoryForBuild(const QString &directory,
                                                               const QString &proFile, int maxdepth = 3);

    bool hasSelection() const;
    bool isTargetSelected(const QString &targetid) const;
    bool isComplete() const;

    bool setupProject(Qt4Project *project);

public slots:
    void setProFilePath(const QString &dir);

private slots:
    void itemWasChanged();
    void addShadowBuildLocation();

private:
    void resetInfos();
    QPair<QIcon, QString> reportIssues(QtVersion *version, const QString &buildDir);

    QList<ImportInfo> m_infos;
    bool m_preferMobile;
    QString m_proFilePath;
    QString m_defaultShadowBuildLocation;

    Ui::TargetSetupPage *m_ui;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // TARGETSETUPPAGE_H
