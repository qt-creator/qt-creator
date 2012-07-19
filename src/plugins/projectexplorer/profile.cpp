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

#include "profile.h"

#include "devicesupport/devicemanager.h"
#include "profileinformation.h"
#include "profilemanager.h"
#include "project.h"
#include "toolchainmanager.h"

#include <QApplication>
#include <QIcon>
#include <QStyle>
#include <QTextStream>
#include <QUuid>

namespace {

const char ID_KEY[] = "PE.Profile.Id";
const char DISPLAYNAME_KEY[] = "PE.Profile.Name";
const char AUTODETECTED_KEY[] = "PE.Profile.AutoDetected";
const char DATA_KEY[] = "PE.Profile.Data";
const char ICON_KEY[] = "PE.Profile.Icon";

} // namespace

namespace ProjectExplorer {

// -------------------------------------------------------------------------
// ProfilePrivate
// -------------------------------------------------------------------------

namespace Internal {

class ProfilePrivate
{
public:
    ProfilePrivate() :
        m_id(QUuid::createUuid().toString().toLatin1().constData()),
        m_autodetected(false),
        m_isValid(true)
    { }

    QString m_displayName;
    Core::Id m_id;
    bool m_autodetected;
    bool m_isValid;
    QIcon m_icon;
    QString m_iconPath;

    QHash<Core::Id, QVariant> m_data;
};

} // namespace Internal

// -------------------------------------------------------------------------
// Profile:
// -------------------------------------------------------------------------

Profile::Profile() :
    d(new Internal::ProfilePrivate)
{
    ProfileManager *stm = ProfileManager::instance();
    foreach (ProfileInformation *sti, stm->profileInformation())
        d->m_data.insert(sti->dataId(), sti->defaultValue(this));

    setDisplayName(QCoreApplication::translate("ProjectExplorer::Profile", "Unnamed"));
    setIconPath(QLatin1String(":///DESKTOP///"));
}

Profile::~Profile()
{
    delete d;
}

Profile *Profile::clone() const
{
    Profile *p = new Profile;
    p->d->m_displayName = QCoreApplication::translate("ProjectExplorer::Profile", "Clone of %1")
            .arg(d->m_displayName);
    p->d->m_autodetected = false;
    p->d->m_data = d->m_data;
    p->d->m_isValid = d->m_isValid;
    p->d->m_icon = d->m_icon;
    p->d->m_iconPath = d->m_iconPath;
    return p;
}

bool Profile::isValid() const
{
    return d->m_id.isValid() && d->m_isValid;
}

QList<Task> Profile::validate()
{
    QList<Task> result;
    QList<ProfileInformation *> infoList = ProfileManager::instance()->profileInformation();
    foreach (ProfileInformation *i, infoList)
        result.append(i->validate(this));
    return result;
}

QString Profile::displayName() const
{
    return d->m_displayName;
}

void Profile::setDisplayName(const QString &name)
{
    // make name unique:
    QStringList nameList;
    foreach (Profile *p, ProfileManager::instance()->profiles())
        nameList << p->displayName();

    QString uniqueName = Project::makeUnique(name, nameList);
    if (uniqueName != name) {
        ToolChain *tc = ToolChainProfileInformation::toolChain(this);
        if (tc) {
            const QString tcPostfix = QString::fromLatin1("-%1").arg(tc->displayName());
            if (!name.contains(tcPostfix))
                uniqueName = Project::makeUnique(name + tcPostfix, nameList);
        }
    }

    if (d->m_displayName == uniqueName)
        return;
    d->m_displayName = uniqueName;
    profileUpdated();
}

bool Profile::isAutoDetected() const
{
    return d->m_autodetected;
}

Core::Id Profile::id() const
{
    return d->m_id;
}

QIcon Profile::icon() const
{
    return d->m_icon;
}

QString Profile::iconPath() const
{
    return d->m_iconPath;
}

void Profile::setIconPath(const QString &path)
{
    if (d->m_iconPath == path)
        return;
    d->m_iconPath = path;
    if (path.isNull())
        d->m_icon = QIcon();
    else if (path == QLatin1String(":///DESKTOP///"))
        d->m_icon = qApp->style()->standardIcon(QStyle::SP_ComputerIcon);
    else
        d->m_icon = QIcon(path);
    profileUpdated();
}

QVariant Profile::value(const Core::Id &key, const QVariant &unset) const
{
    return d->m_data.value(key, unset);
}

bool Profile::hasValue(const Core::Id &key) const
{
    return d->m_data.contains(key);
}

void Profile::setValue(const Core::Id &key, const QVariant &value)
{
    if (d->m_data.value(key) == value)
        return;
    d->m_data.insert(key, value);
    profileUpdated();
}

void Profile::removeKey(const Core::Id &key)
{
    if (!d->m_data.contains(key))
        return;
    d->m_data.remove(key);
    profileUpdated();
}

QVariantMap Profile::toMap() const
{
    QVariantMap data;
    data.insert(QLatin1String(ID_KEY), QString::fromLatin1(d->m_id.name()));
    data.insert(QLatin1String(DISPLAYNAME_KEY), d->m_displayName);
    data.insert(QLatin1String(AUTODETECTED_KEY), d->m_autodetected);
    data.insert(QLatin1String(ICON_KEY), d->m_iconPath);

    QVariantMap extra;
    foreach (const Core::Id &key, d->m_data.keys())
        extra.insert(QString::fromLatin1(key.name().constData()), d->m_data.value(key));
    data.insert(QLatin1String(DATA_KEY), extra);

    return data;
}

bool Profile::operator==(const Profile &other) const
{
    return d->m_data == other.d->m_data;
}

void Profile::addToEnvironment(Utils::Environment &env) const
{
    QList<ProfileInformation *> infoList = ProfileManager::instance()->profileInformation();
    foreach (ProfileInformation *si, infoList)
        si->addToEnvironment(this, env);
}

QString Profile::toHtml()
{
    QString rc;
    QTextStream str(&rc);
    str << "<html><body>";
    str << "<h3>" << displayName() << "</h3>";
    str << "<table>";

    if (!isValid()) {
        QList<Task> issues = validate();
        str << "<p>";
        foreach (const Task &t, issues) {
            str << "<b>";
            switch (t.type) {
            case Task::Error:
                QCoreApplication::translate("ProjectExplorer::Profile", "Error:");
                break;
            case Task::Warning:
                QCoreApplication::translate("ProjectExplorer::Profile", "Warning:");
                break;
            case Task::Unknown:
            default:
                break;
            }
            str << "</b>" << t.description << "<br>";
        }
        str << "</p>";
    }

    QList<ProfileInformation *> infoList = ProfileManager::instance()->profileInformation();
    foreach (ProfileInformation *i, infoList) {
        ProfileInformation::ItemList list = i->toUserOutput(this);
        foreach (const ProfileInformation::Item &j, list)
            str << "<tr><td><b>" << j.first << ":</b></td><td>" << j.second << "</td></tr>";
    }
    str << "</table></body></html>";
    return rc;
}

bool Profile::fromMap(const QVariantMap &data)
{
    const QString id = data.value(QLatin1String(ID_KEY)).toString();
    if (id.isEmpty())
        return false;
    d->m_id = Core::Id(id);
    d->m_displayName = data.value(QLatin1String(DISPLAYNAME_KEY)).toString();
    d->m_autodetected = data.value(QLatin1String(AUTODETECTED_KEY)).toBool();
    setIconPath(data.value(QLatin1String(ICON_KEY)).toString());

    QVariantMap extra = data.value(QLatin1String(DATA_KEY)).toMap();
    foreach (const QString &key, extra.keys())
        d->m_data.insert(Core::Id(key), extra.value(key));

    return true;
}

void Profile::setAutoDetected(bool detected)
{
    d->m_autodetected = detected;
}

void Profile::setValid(bool valid)
{
    d->m_isValid = valid;
}

void Profile::profileUpdated()
{
    ProfileManager::instance()->notifyAboutUpdate(this);
}

} // namespace ProjectExplorer
