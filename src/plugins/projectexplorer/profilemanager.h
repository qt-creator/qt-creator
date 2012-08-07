/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#ifndef PROFILEMANAGER_H
#define PROFILEMANAGER_H

#include "projectexplorer_export.h"

#include "task.h"

#include <coreplugin/id.h>
#include <utils/fileutils.h>

#include <QObject>
#include <QPair>

namespace Utils { class Environment; }

namespace ProjectExplorer {
class Profile;
class ProfileConfigWidget;

namespace Internal {
class ProfileManagerPrivate;
class ProfileModel;
} // namespace Internal

/**
 * @brief The ProfileInformation class
 *
 * One piece of information stored in the profile.
 *
 * This needs to get registered with the \a ProfileManager.
 */
class PROJECTEXPLORER_EXPORT ProfileInformation : public QObject
{
    Q_OBJECT

public:
    typedef QPair<QString, QString> Item;
    typedef QList<Item> ItemList;

    virtual Core::Id dataId() const = 0;

    virtual unsigned int priority() const = 0; // the higher the closer to the top.

    virtual bool visibleIn(Profile *) { return true; }
    virtual QVariant defaultValue(Profile *) const = 0;

    virtual QList<Task> validate(Profile *) const = 0;

    virtual ItemList toUserOutput(Profile *p) const = 0;

    virtual ProfileConfigWidget *createConfigWidget(Profile *) const = 0;

    virtual void addToEnvironment(const Profile *p, Utils::Environment &env) const;

    virtual QString displayNamePostfix(const Profile *p) const;

signals:
    void validationNeeded();
};

class PROJECTEXPLORER_EXPORT ProfileMatcher
{
public:
    virtual ~ProfileMatcher() { }
    virtual bool matches(const Profile *p) const = 0;
};

class PROJECTEXPLORER_EXPORT ProfileManager : public QObject
{
    Q_OBJECT

public:
    static ProfileManager *instance();
    ~ProfileManager();

    QList<Profile *> profiles(const ProfileMatcher *m = 0) const;
    Profile *find(const Core::Id &id) const;
    Profile *find(const ProfileMatcher *m) const;
    Profile *defaultProfile();

    QList<ProfileInformation *> profileInformation() const;

    ProfileConfigWidget *createConfigWidget(Profile *p) const;

public slots:
    bool registerProfile(ProjectExplorer::Profile *p);
    void deregisterProfile(ProjectExplorer::Profile *p);
    QList<Task> validateProfile(ProjectExplorer::Profile *p);
    void setDefaultProfile(ProjectExplorer::Profile *p);

    void saveProfiles();

    void registerProfileInformation(ProjectExplorer::ProfileInformation *pi);
    void deregisterProfileInformation(ProjectExplorer::ProfileInformation *pi);

signals:
    void profileAdded(ProjectExplorer::Profile *);
    // Profile is still valid when this call happens!
    void profileRemoved(ProjectExplorer::Profile *);
    // Profile was updated.
    void profileUpdated(ProjectExplorer::Profile *);
    // Default profile was changed.
    void defaultProfileChanged();
    // Something changed.
    void profilesChanged();

private slots:
    void validateProfiles();

private:
    explicit ProfileManager(QObject *parent = 0);

    // Make sure the this is only called after all
    // ProfileInformation are registered!
    void restoreProfiles();
    class ProfileList
    {
    public:
        ProfileList()
        { }
        Core::Id defaultProfile;
        QList<Profile *> profiles;
    };
    ProfileList restoreProfiles(const QString &fileName);

    void notifyAboutUpdate(ProjectExplorer::Profile *p);
    void addProfile(Profile *p);

    Internal::ProfileManagerPrivate *const d;

    static ProfileManager *m_instance;

    friend class Internal::ProfileManagerPrivate; // for the restoreToolChains methods
    friend class ProjectExplorerPlugin; // for constructor
    friend class Profile;
    friend class Internal::ProfileModel;
};

} // namespace ProjectExplorer

#endif // PROFILEMANAGER_H
