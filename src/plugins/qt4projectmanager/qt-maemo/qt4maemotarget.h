/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
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
class MaemoPerTargetDeviceConfigurationListModel;
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
    virtual QString packageName() const=0;
    virtual QString shortDescription() const=0;
    virtual QString packageFileName() const=0;

    bool setProjectVersion(const QString &version, QString *error = 0);
    bool setPackageName(const QString &packageName);
    bool setShortDescription(const QString &description);

    struct DebugArchitecture {
        explicit DebugArchitecture(const QString &a = QString(), const QString &t = QString()) :
            architecture(a), gnuTarget(t)
        { }

        QString architecture;
        QString gnuTarget;
    };
    // TODO: Is this needed with the ABI info we have?
    DebugArchitecture debugArchitecture() const;

    MaemoPerTargetDeviceConfigurationListModel *deviceConfigurationsModel() const {
        return m_deviceConfigurationsListModel;
    }
    QList<ProjectExplorer::ToolChain *> possibleToolChains(ProjectExplorer::BuildConfiguration *bc) const;

protected:
    enum ActionStatus { NoActionRequired, ActionSuccessful, ActionFailed };

    void initDeviceConfigurationsModel();
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
    virtual bool setPackageNameInternal(const QString &packageName)=0;
    virtual bool setShortDescriptionInternal(const QString &description)=0;
    virtual ActionStatus createSpecialTemplates()=0;
    virtual void handleTargetAddedSpecial()=0;
    virtual bool targetCanBeRemoved() const=0;
    virtual void removeTarget()=0;
    virtual QStringList packagingFilePaths() const=0;

    ActionStatus createTemplates();
    bool initPackagingSettingsFromOtherTarget();
    virtual bool initAdditionalPackagingSettingsFromOtherTarget()=0;

    Qt4BuildConfigurationFactory *m_buildConfigurationFactory;
    Qt4MaemoDeployConfigurationFactory *m_deployConfigurationFactory;
    MaemoPerTargetDeviceConfigurationListModel * m_deviceConfigurationsListModel;
};


class AbstractDebBasedQt4MaemoTarget : public AbstractQt4MaemoTarget
{
    Q_OBJECT
public:
    AbstractDebBasedQt4MaemoTarget(Qt4Project *parent, const QString &id);
    ~AbstractDebBasedQt4MaemoTarget();

    QString debianDirPath() const;
    QStringList debianFiles() const;

    virtual QString debianDirName() const=0;
    virtual QString projectVersion(QString *error = 0) const;
    virtual QString packageName() const;
    virtual QString shortDescription() const;
    virtual QString packageFileName() const;

    bool setPackageManagerIcon(const QString &iconFilePath, QString *error = 0);
    QIcon packageManagerIcon(QString *error = 0) const;
    bool setPackageManagerName(const QString &name, QString *error = 0);
    QString packageManagerName() const;

signals:
    void debianDirContentsChanged();
    void changeLogChanged();
    void controlChanged();

protected:
    bool adaptControlFileField(QByteArray &document, const QByteArray &fieldName,
        const QByteArray &newFieldValue);

private slots:
    void handleDebianDirContentsChanged();
    void handleDebianFileChanged(const QString &filePath);

private:
    virtual bool setProjectVersionInternal(const QString &version,
        QString *error = 0);
    virtual bool setPackageNameInternal(const QString &packageName);
    virtual bool setShortDescriptionInternal(const QString &description);

    virtual ActionStatus createSpecialTemplates();
    virtual void handleTargetAddedSpecial();
    virtual bool targetCanBeRemoved() const;
    virtual void removeTarget();
    virtual bool initAdditionalPackagingSettingsFromOtherTarget();
    virtual QStringList packagingFilePaths() const;

    virtual void addAdditionalControlFileFields(QByteArray &controlContents)=0;
    virtual QByteArray packageManagerNameFieldName() const=0;

    QString changeLogFilePath() const;
    QString controlFilePath() const;
    QByteArray controlFileFieldValue(const QString &key, bool multiLine) const;
    bool setControlFieldValue(const QByteArray &fieldName,
        const QByteArray &fieldValue);
    bool adaptRulesFile();
    bool adaptControlFile();
    bool setPackageManagerIconInternal(const QString &iconFilePath,
        QString *error = 0);
    QString defaultPackageFileName() const;
    bool setPackageManagerNameInternal(const QString &name, QString *error = 0);
};


class AbstractRpmBasedQt4MaemoTarget : public AbstractQt4MaemoTarget
{
    Q_OBJECT
public:
    AbstractRpmBasedQt4MaemoTarget(Qt4Project *parent, const QString &id);
    ~AbstractRpmBasedQt4MaemoTarget();

    virtual bool allowsRemoteMounts() const { return false; }
    virtual bool allowsPackagingDisabling() const { return false; }
    virtual bool allowsQmlDebugging() const { return true; }

    virtual QString projectVersion(QString *error = 0) const;
    virtual QString packageName() const;
    virtual QString shortDescription() const;
    virtual QString packageFileName() const;

    QString specFilePath() const;

signals:
    void specFileChanged();

private:
    virtual bool setProjectVersionInternal(const QString &version,
        QString *error = 0);
    virtual bool setPackageNameInternal(const QString &packageName);
    virtual bool setShortDescriptionInternal(const QString &description);
    virtual ActionStatus createSpecialTemplates();
    virtual void handleTargetAddedSpecial();
    virtual bool targetCanBeRemoved() const;
    virtual void removeTarget();
    virtual bool initAdditionalPackagingSettingsFromOtherTarget();
    virtual QStringList packagingFilePaths() const { return QStringList(specFilePath()); }

    virtual QString specFileName() const=0;

    QByteArray getValueForTag(const QByteArray &tag, QString *error) const;
    bool setValueForTag(const QByteArray &tag, const QByteArray &value,
        QString *error);
};


class Qt4Maemo5Target : public AbstractDebBasedQt4MaemoTarget
{
    Q_OBJECT
public:
    explicit Qt4Maemo5Target(Qt4Project *parent, const QString &id);
    virtual ~Qt4Maemo5Target();

    virtual bool allowsRemoteMounts() const { return true; }
    virtual bool allowsPackagingDisabling() const { return true; }
    virtual bool allowsQmlDebugging() const { return false; }

    static QString defaultDisplayName();

private:
    virtual void addAdditionalControlFileFields(QByteArray &controlContents);
    virtual QString debianDirName() const;
    virtual QByteArray packageManagerNameFieldName() const;
};


class Qt4HarmattanTarget : public AbstractDebBasedQt4MaemoTarget
{
    Q_OBJECT
public:
    explicit Qt4HarmattanTarget(Qt4Project *parent, const QString &id);
    virtual ~Qt4HarmattanTarget();

    virtual bool allowsRemoteMounts() const { return false; }
    virtual bool allowsPackagingDisabling() const { return false; }
    virtual bool allowsQmlDebugging() const { return true; }

    static QString defaultDisplayName();

private:
    virtual void addAdditionalControlFileFields(QByteArray &controlContents);
    virtual QString debianDirName() const;
    virtual QByteArray packageManagerNameFieldName() const;
};


class Qt4MeegoTarget : public AbstractRpmBasedQt4MaemoTarget
{
    Q_OBJECT
public:
    explicit Qt4MeegoTarget(Qt4Project *parent, const QString &id);
    virtual ~Qt4MeegoTarget();
    static QString defaultDisplayName();
private:
    virtual QString specFileName() const;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // QT4MAEMOTARGET_H
