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

#ifndef MAEMOTEMPLATESCREATOR_H
#define MAEMOTEMPLATESCREATOR_H

#include <QtCore/QFile>
#include <QtCore/QMap>
#include <QtCore/QObject>
#include <QtCore/QSharedPointer>
#include <QtGui/QIcon>

QT_FORWARD_DECLARE_CLASS(QFileSystemWatcher)

namespace ProjectExplorer {
class Project;
class ProjectNode;
class Target;
}

namespace Qt4ProjectManager {
class Qt4Project;
class Qt4Target;

namespace Internal {
class Qt4ProFileNode;

class MaemoTemplatesManager : public QObject
{
    Q_OBJECT

public:
    static MaemoTemplatesManager *instance(QObject *parent = 0);

    QString version(const ProjectExplorer::Project *project,
        QString *error) const;
    bool setVersion(const ProjectExplorer::Project *project,
        const QString &version, QString *error) const;

    QString debianDirPath(const ProjectExplorer::Project *project) const;
    QStringList debianFiles(const ProjectExplorer::Project *project) const;

    QIcon packageManagerIcon(const ProjectExplorer::Project *project,
        QString *error) const;
    bool setPackageManagerIcon(const ProjectExplorer::Project *project,
        const QString &iconFilePath, QString *error) const;

    QString name(const ProjectExplorer::Project *project) const;
    bool setName(const ProjectExplorer::Project *project,
        const QString &name);

    QString shortDescription(const ProjectExplorer::Project *project) const;
    bool setShortDescription(const ProjectExplorer::Project *project,
        const QString &description);

signals:
    void debianDirContentsChanged(const ProjectExplorer::Project *project);
    void changeLogChanged(const ProjectExplorer::Project *project);
    void controlChanged(const ProjectExplorer::Project *project);

private slots:
    void handleActiveProjectChanged(ProjectExplorer::Project *project);
    bool handleTarget(ProjectExplorer::Target *target);
    void handleDebianDirContentsChanged();
    void handleDebianFileChanged(const QString &filePath);
    void handleProjectToBeRemoved(ProjectExplorer::Project *project);
    void handleProFileUpdated();

private:
    explicit MaemoTemplatesManager(QObject *parent);
    void raiseError(const QString &reason);
    QString changeLogFilePath(const ProjectExplorer::Project *project) const;
    QString controlFilePath(const ProjectExplorer::Project *project) const;
    bool createDebianTemplatesIfNecessary(const ProjectExplorer::Target *target);
    bool updateDesktopFiles(const Qt4Target *target);
    bool updateDesktopFile(const Qt4Target *target,
        Qt4ProFileNode *proFileNode);
    ProjectExplorer::Project *findProject(const QFileSystemWatcher *fsWatcher) const;
    void findLine(const QByteArray &string, QByteArray &document,
        int &lineEndPos, int &valuePos);
    bool adaptRulesFile(const ProjectExplorer::Project *project);
    bool adaptControlFile(const ProjectExplorer::Project *project);
    bool adaptControlFileField(QByteArray &document, const QByteArray &fieldName,
        const QByteArray &newFieldValue);
    QSharedPointer<QFile> openFile(const QString &filePath,
        QIODevice::OpenMode mode, QString *error) const;
    bool setFieldValue(const ProjectExplorer::Project *project,
        const QByteArray &fieldName, const QByteArray &fieldValue);
    QByteArray controlFileFieldValue(const ProjectExplorer::Project *project,
        const QString &key, bool multiLine) const;

    static MaemoTemplatesManager *m_instance;

    typedef QMap<ProjectExplorer::Project *, QFileSystemWatcher *> MaemoProjectMap;
    MaemoProjectMap m_maemoProjects;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // MAEMOTEMPLATESCREATOR_H
