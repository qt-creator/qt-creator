/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef QT4MAEMOTARGET_H
#define QT4MAEMOTARGET_H

#include "qt4target.h"

#include <QtCore/QIODevice>
#include <QtCore/QSharedPointer>
#include <QtGui/QIcon>

QT_FORWARD_DECLARE_CLASS(QFile)
QT_FORWARD_DECLARE_CLASS(QFileSystemWatcher)

namespace Qt4ProjectManager {
class Qt4Project;
namespace Internal {
class Qt4MaemoDeployConfigurationFactory;

class AbstractQt4MaemoTarget : public Qt4BaseTarget
{
    friend class Qt4MaemoTargetFactory;
    Q_OBJECT
public:
    explicit AbstractQt4MaemoTarget(Qt4Project *parent, const QString &id);
    virtual ~AbstractQt4MaemoTarget();

    Internal::Qt4BuildConfigurationFactory *buildConfigurationFactory() const;
    ProjectExplorer::DeployConfigurationFactory *deployConfigurationFactory() const;
    QString defaultBuildDirectory() const;
    void createApplicationProFiles();

    QString debianDirPath() const;
    QStringList debianFiles() const;

    QString projectVersion(QString *error = 0) const;
    bool setProjectVersion(const QString &version, QString *error = 0);

    QIcon packageManagerIcon(QString *error = 0) const;
    bool setPackageManagerIcon(const QString &iconFilePath, QString *error = 0);

    QString name() const;
    bool setName(const QString &name);

    QString shortDescription() const;
    bool setShortDescription(const QString &description);

    virtual bool allowsRemoteMounts() const=0;
    virtual bool allowsPackagingDisabling() const=0;
    virtual bool allowsQmlDebugging() const=0;

signals:
    void debianDirContentsChanged();
    void changeLogChanged();
    void controlChanged();

private slots:
    void handleTargetAdded(ProjectExplorer::Target *target);
    void handleTargetToBeRemoved(ProjectExplorer::Target *target);
    void handleDebianDirContentsChanged();
    void handleDebianFileChanged(const QString &filePath);

private:
    virtual QString debianDirName() const=0;

    bool setProjectVersionInternal(const QString &version, QString *error = 0);
    bool setPackageManagerIconInternal(const QString &iconFilePath,
        QString *error = 0);
    bool setNameInternal(const QString &name);
    bool setShortDescriptionInternal(const QString &description);

    QString changeLogFilePath() const;
    QString controlFilePath() const;
    QByteArray controlFileFieldValue(const QString &key, bool multiLine) const;
    bool setControlFieldValue(const QByteArray &fieldName,
        const QByteArray &fieldValue);
    bool adaptControlFileField(QByteArray &document, const QByteArray &fieldName,
        const QByteArray &newFieldValue);
    QSharedPointer<QFile> openFile(const QString &filePath,
        QIODevice::OpenMode mode, QString *error) const;
    bool createDebianTemplatesIfNecessary();
    bool adaptRulesFile();
    bool adaptControlFile();
    bool initPackagingSettingsFromOtherTarget();
    void raiseError(const QString &reason);

    Qt4BuildConfigurationFactory *m_buildConfigurationFactory;
    Qt4MaemoDeployConfigurationFactory *m_deployConfigurationFactory;
    QFileSystemWatcher * const m_debianFilesWatcher;
};


class Qt4Maemo5Target : public AbstractQt4MaemoTarget
{
    Q_OBJECT
public:
    explicit Qt4Maemo5Target(Qt4Project *parent, const QString &id);
    virtual ~Qt4Maemo5Target();

    static QString defaultDisplayName();

private:
    virtual QString debianDirName() const;
    virtual bool allowsRemoteMounts() const { return true; }
    virtual bool allowsPackagingDisabling() const { return true; }
    virtual bool allowsQmlDebugging() const { return false; }
};

class Qt4HarmattanTarget : public AbstractQt4MaemoTarget
{
    Q_OBJECT
public:
    explicit Qt4HarmattanTarget(Qt4Project *parent, const QString &id);
    virtual ~Qt4HarmattanTarget();

    static QString defaultDisplayName();

private:
    virtual QString debianDirName() const;
    virtual bool allowsRemoteMounts() const { return false; }
    virtual bool allowsPackagingDisabling() const { return false; }
    virtual bool allowsQmlDebugging() const { return true; }
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // QT4MAEMOTARGET_H
