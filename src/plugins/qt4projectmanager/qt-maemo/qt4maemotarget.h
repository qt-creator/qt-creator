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
    QList<ProjectExplorer::RunConfiguration *> runConfigurationsForNode(ProjectExplorer::Node *n);

    virtual bool allowsRemoteMounts() const=0;
    virtual bool allowsPackagingDisabling() const=0;
    virtual bool allowsQmlDebugging() const=0;

    virtual QString projectVersion(QString *error = 0) const=0;
    virtual QIcon packageManagerIcon(QString *error = 0) const=0;
    virtual QString name() const=0;
    virtual QString shortDescription() const=0;

    bool setProjectVersion(const QString &version, QString *error = 0);
    bool setPackageManagerIcon(const QString &iconFilePath, QString *error = 0);
    bool setName(const QString &name);
    bool setShortDescription(const QString &description);

protected:
    void raiseError(const QString &reason);
    QSharedPointer<QFile> openFile(const QString &filePath,
        QIODevice::OpenMode mode, QString *error) const;

    QFileSystemWatcher * const m_filesWatcher;

private slots:
    void handleTargetAdded(ProjectExplorer::Target *target);
    void handleTargetToBeRemoved(ProjectExplorer::Target *target);

private:
    virtual bool setProjectVersionInternal(const QString &version,
        QString *error = 0)=0;
    virtual bool setPackageManagerIconInternal(const QString &iconFilePath,
        QString *error = 0)=0;
    virtual bool setNameInternal(const QString &name)=0;
    virtual bool setShortDescriptionInternal(const QString &description)=0;
    virtual bool createSpecialTemplates()=0;
    virtual void handleTargetAddedSpecial()=0;
    virtual bool targetCanBeRemoved() const=0;
    virtual void removeTarget()=0;

    bool createTemplates();
    bool initPackagingSettingsFromOtherTarget();

    virtual QByteArray rawIconValue() const=0;
    virtual bool setRawIconValue(const QByteArray &icon)=0;

    Qt4BuildConfigurationFactory *m_buildConfigurationFactory;
    Qt4MaemoDeployConfigurationFactory *m_deployConfigurationFactory;
};


class AbstractDebBasedQt4MaemoTarget : public AbstractQt4MaemoTarget
{
    Q_OBJECT
public:
    AbstractDebBasedQt4MaemoTarget(Qt4Project *parent, const QString &id);
    ~AbstractDebBasedQt4MaemoTarget();

    QString debianDirPath() const;
    QStringList debianFiles() const;

signals:
    void debianDirContentsChanged();
    void changeLogChanged();
    void controlChanged();

private slots:
    void handleDebianDirContentsChanged();
    void handleDebianFileChanged(const QString &filePath);

private:
    virtual QString debianDirName() const=0;
    virtual QString projectVersion(QString *error = 0) const;
    virtual QIcon packageManagerIcon(QString *error = 0) const;
    virtual QString name() const;
    virtual QString shortDescription() const;
    virtual bool setProjectVersionInternal(const QString &version,
        QString *error = 0);
    virtual bool setPackageManagerIconInternal(const QString &iconFilePath,
        QString *error = 0);
    virtual bool setNameInternal(const QString &name);
    virtual bool setShortDescriptionInternal(const QString &description);

    virtual bool createSpecialTemplates();
    virtual void handleTargetAddedSpecial();
    virtual bool targetCanBeRemoved() const;
    virtual void removeTarget();

    virtual QByteArray rawIconValue() const;
    virtual bool setRawIconValue(const QByteArray &icon);

    QString changeLogFilePath() const;
    QString controlFilePath() const;
    QByteArray controlFileFieldValue(const QString &key, bool multiLine) const;
    bool setControlFieldValue(const QByteArray &fieldName,
        const QByteArray &fieldValue);
    bool adaptControlFileField(QByteArray &document, const QByteArray &fieldName,
        const QByteArray &newFieldValue);
    bool adaptRulesFile();
    bool adaptControlFile();
};

class Qt4Maemo5Target : public AbstractDebBasedQt4MaemoTarget
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

class Qt4HarmattanTarget : public AbstractDebBasedQt4MaemoTarget
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
