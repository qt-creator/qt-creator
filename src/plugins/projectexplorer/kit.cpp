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

#include "kit.h"

#include "kitmanager.h"
#include "ioutputparser.h"

#include <QApplication>
#include <QIcon>
#include <QStyle>
#include <QTextStream>
#include <QUuid>

using namespace Core;

namespace {

const char ID_KEY[] = "PE.Profile.Id";
const char DISPLAYNAME_KEY[] = "PE.Profile.Name";
const char AUTODETECTED_KEY[] = "PE.Profile.AutoDetected";
const char SDK_PROVIDED_KEY[] = "PE.Profile.SDK";
const char DATA_KEY[] = "PE.Profile.Data";
const char ICON_KEY[] = "PE.Profile.Icon";

} // namespace

namespace ProjectExplorer {

// --------------------------------------------------------------------
// Helper:
// --------------------------------------------------------------------

static QString cleanName(const QString &name)
{
    QString result = name;
    result.replace(QRegExp(QLatin1String("\\W")), QLatin1String("_"));
    result.replace(QRegExp(QLatin1String("_+")), QLatin1String("_")); // compact _
    result.remove(QRegExp(QLatin1String("^_*"))); // remove leading _
    result.remove(QRegExp(QLatin1String("_+$"))); // remove trailing _
    if (result.isEmpty())
        result = QLatin1String("unknown");
    return result;
}

// -------------------------------------------------------------------------
// KitPrivate
// -------------------------------------------------------------------------

namespace Internal {

class KitPrivate
{
public:
    KitPrivate(Id id) :
        m_id(id),
        m_autodetected(false),
        m_sdkProvided(false),
        m_isValid(true),
        m_hasWarning(false),
        m_nestedBlockingLevel(0),
        m_mustNotify(false),
        m_mustNotifyAboutDisplayName(false)
    {
        if (!id.isValid())
            m_id = Id::fromString(QUuid::createUuid().toString());
    }

    QString m_displayName;
    Id m_id;
    bool m_autodetected;
    bool m_sdkProvided;
    bool m_isValid;
    bool m_hasWarning;
    QIcon m_icon;
    QString m_iconPath;
    int m_nestedBlockingLevel;
    bool m_mustNotify;
    bool m_mustNotifyAboutDisplayName;

    QHash<Core::Id, QVariant> m_data;
    QSet<Core::Id> m_sticky;
};

} // namespace Internal

// -------------------------------------------------------------------------
// Kit:
// -------------------------------------------------------------------------

Kit::Kit(Core::Id id) :
    d(new Internal::KitPrivate(id))
{
    KitManager *stm = KitManager::instance();
    KitGuard g(this);
    foreach (KitInformation *sti, stm->kitInformation())
        setValue(sti->dataId(), sti->defaultValue(this));

    setDisplayName(QCoreApplication::translate("ProjectExplorer::Kit", "Unnamed"));
    setIconPath(QLatin1String(":///DESKTOP///"));
}

Kit::~Kit()
{
    delete d;
}

void Kit::blockNotification()
{
    ++d->m_nestedBlockingLevel;
}

void Kit::unblockNotification()
{
    --d->m_nestedBlockingLevel;
    if (d->m_nestedBlockingLevel > 0)
        return;
    if (d->m_mustNotifyAboutDisplayName)
        kitDisplayNameChanged();
    else if (d->m_mustNotify)
        kitUpdated();
    d->m_mustNotify = false;
    d->m_mustNotifyAboutDisplayName = false;
}

Kit *Kit::clone(bool keepName) const
{
    Kit *k = new Kit;
    if (keepName)
        k->d->m_displayName = d->m_displayName;
    else
        k->d->m_displayName = QCoreApplication::translate("ProjectExplorer::Kit", "Clone of %1")
                .arg(d->m_displayName);
    k->d->m_autodetected = false;
    k->d->m_data = d->m_data;
    k->d->m_isValid = d->m_isValid;
    k->d->m_icon = d->m_icon;
    k->d->m_iconPath = d->m_iconPath;
    k->d->m_sticky = d->m_sticky;
    return k;
}

void Kit::copyFrom(const Kit *k)
{
    KitGuard g(this);
    d->m_data = k->d->m_data;
    d->m_iconPath = k->d->m_iconPath;
    d->m_icon = k->d->m_icon;
    d->m_autodetected = k->d->m_autodetected;
    d->m_displayName = k->d->m_displayName;
    d->m_mustNotify = true;
    d->m_mustNotifyAboutDisplayName = true;
    d->m_sticky = k->d->m_sticky;
}

bool Kit::isValid() const
{
    return d->m_id.isValid() && d->m_isValid;
}

bool Kit::hasWarning() const
{
    return d->m_hasWarning;
}

QList<Task> Kit::validate() const
{
    QList<Task> result;
    QList<KitInformation *> infoList = KitManager::instance()->kitInformation();
    d->m_isValid = true;
    d->m_hasWarning = false;
    foreach (KitInformation *i, infoList) {
        QList<Task> tmp = i->validate(this);
        foreach (const Task &t, tmp) {
            if (t.type == Task::Error)
                d->m_isValid = false;
            if (t.type == Task::Warning)
                d->m_hasWarning = true;
        }
        result.append(tmp);
    }
    qSort(result);
    return result;
}

void Kit::fix()
{
    KitGuard g(this);
    foreach (KitInformation *i, KitManager::instance()->kitInformation())
        i->fix(this);
}

void Kit::setup()
{
    KitGuard g(this);
    // Process the KitInfos in reverse order: They may only be based on other information lower in
    // the stack.
    QList<KitInformation *> info = KitManager::instance()->kitInformation();
    for (int i = info.count() - 1; i >= 0; --i)
        info.at(i)->setup(this);
}

QString Kit::displayName() const
{
    return d->m_displayName;
}

static QString candidateName(const QString &name, const QString &postfix)
{
    if (name.contains(postfix))
        return QString();
    QString candidate = name;
    if (!candidate.isEmpty())
        candidate.append(QLatin1Char('-'));
    candidate.append(postfix);
    return candidate;
}

void Kit::setDisplayName(const QString &name)
{
    if (d->m_displayName == name)
        return;
    d->m_displayName = name;
    kitDisplayNameChanged();
}

QStringList Kit::candidateNameList(const QString &base) const
{
    QStringList result;
    result << base;
    foreach (KitInformation *ki, KitManager::instance()->kitInformation()) {
        const QString postfix = ki->displayNamePostfix(this);
        if (!postfix.isEmpty())
            result << candidateName(base, postfix);
    }
    return result;
}

QString Kit::fileSystemFriendlyName() const
{
    QString name = cleanName(displayName());
    foreach (Kit *i, KitManager::instance()->kits()) {
        if (i == this)
            continue;
        if (name == cleanName(i->displayName())) {
            // append part of the kit id: That should be unique enough;-)
            // Leading { will be turned into _ which should be fine.
            name = cleanName(name + QLatin1Char('_') + (id().toString().left(7)));
            break;
        }
    }
    return name;
}

bool Kit::isAutoDetected() const
{
    return d->m_autodetected;
}

bool Kit::isSdkProvided() const
{
    return d->m_sdkProvided;
}

Id Kit::id() const
{
    return d->m_id;
}

QIcon Kit::icon() const
{
    return d->m_icon;
}

QString Kit::iconPath() const
{
    return d->m_iconPath;
}

void Kit::setIconPath(const QString &path)
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
    kitUpdated();
}

QVariant Kit::value(Id key, const QVariant &unset) const
{
    return d->m_data.value(key, unset);
}

bool Kit::hasValue(Id key) const
{
    return d->m_data.contains(key);
}

void Kit::setValue(Id key, const QVariant &value)
{
    if (d->m_data.value(key) == value)
        return;
    d->m_data.insert(key, value);
    kitUpdated();
}

void Kit::removeKey(Id key)
{
    if (!d->m_data.contains(key))
        return;
    d->m_data.remove(key);
    d->m_sticky.remove(key);
    kitUpdated();
}

bool Kit::isSticky(Core::Id id) const
{
    return d->m_sticky.contains(id);
}

bool Kit::isDataEqual(const Kit *other) const
{
    return d->m_data == other->d->m_data;
}

bool Kit::isEqual(const Kit *other) const
{
    return isDataEqual(other)
            && d->m_iconPath == other->d->m_iconPath
            && d->m_displayName == other->d->m_displayName;
}

QVariantMap Kit::toMap() const
{
    typedef QHash<Core::Id, QVariant>::ConstIterator IdVariantConstIt;

    QVariantMap data;
    data.insert(QLatin1String(ID_KEY), QString::fromLatin1(d->m_id.name()));
    data.insert(QLatin1String(DISPLAYNAME_KEY), d->m_displayName);
    data.insert(QLatin1String(AUTODETECTED_KEY), d->m_autodetected);
    data.insert(QLatin1String(SDK_PROVIDED_KEY), d->m_sdkProvided);
    data.insert(QLatin1String(ICON_KEY), d->m_iconPath);

    QVariantMap extra;

    const IdVariantConstIt cend = d->m_data.constEnd();
    for (IdVariantConstIt it = d->m_data.constBegin(); it != cend; ++it)
        extra.insert(QString::fromLatin1(it.key().name().constData()), it.value());
    data.insert(QLatin1String(DATA_KEY), extra);

    return data;
}

void Kit::addToEnvironment(Utils::Environment &env) const
{
    QList<KitInformation *> infoList = KitManager::instance()->kitInformation();
    foreach (KitInformation *ki, infoList)
        ki->addToEnvironment(this, env);
}

IOutputParser *Kit::createOutputParser() const
{
    IOutputParser *first = 0;
    QList<KitInformation *> infoList = KitManager::instance()->kitInformation();
    foreach (KitInformation *ki, infoList) {
        IOutputParser *next = ki->createOutputParser(this);
        if (!first)
            first = next;
        else
            first->appendOutputParser(next);
    }
    return first;
}

QString Kit::toHtml() const
{
    QString rc;
    QTextStream str(&rc);
    str << "<html><body>";
    str << "<h3>" << displayName() << "</h3>";
    str << "<table>";

    if (!isValid() || hasWarning()) {
        QList<Task> issues = validate();
        str << "<p>";
        foreach (const Task &t, issues) {
            str << "<b>";
            switch (t.type) {
            case Task::Error:
                str << QCoreApplication::translate("ProjectExplorer::Kit", "Error:") << " ";
                break;
            case Task::Warning:
                str << QCoreApplication::translate("ProjectExplorer::Kit", "Warning:") << " ";
                break;
            case Task::Unknown:
            default:
                break;
            }
            str << "</b>" << t.description << "<br>";
        }
        str << "</p>";
    }

    QList<KitInformation *> infoList = KitManager::instance()->kitInformation();
    foreach (KitInformation *ki, infoList) {
        KitInformation::ItemList list = ki->toUserOutput(this);
        foreach (const KitInformation::Item &j, list)
            str << "<tr><td><b>" << j.first << ":</b></td><td>" << j.second << "</td></tr>";
    }
    str << "</table></body></html>";
    return rc;
}

bool Kit::fromMap(const QVariantMap &data)
{
    KitGuard g(this);
    Id id = Id::fromSetting(data.value(QLatin1String(ID_KEY)));
    if (!id.isValid())
        return false;
    d->m_id = id;
    d->m_autodetected = data.value(QLatin1String(AUTODETECTED_KEY)).toBool();
    // if we don't have that setting assume that autodetected implies sdk
    QVariant value = data.value(QLatin1String(SDK_PROVIDED_KEY));
    if (value.isValid())
        d->m_sdkProvided = value.toBool();
    else
        d->m_sdkProvided = d->m_autodetected;
    setDisplayName(data.value(QLatin1String(DISPLAYNAME_KEY)).toString());
    setIconPath(data.value(QLatin1String(ICON_KEY)).toString());

    QVariantMap extra = data.value(QLatin1String(DATA_KEY)).toMap();
    d->m_data.clear(); // remove default values
    const QVariantMap::ConstIterator cend = extra.constEnd();
    for (QVariantMap::ConstIterator it = extra.constBegin(); it != cend; ++it)
        setValue(Id::fromString(it.key()), it.value());

    return true;
}

void Kit::setAutoDetected(bool detected)
{
    d->m_autodetected = detected;
}

void Kit::setSdkProvided(bool sdkProvided)
{
    d->m_sdkProvided = sdkProvided;
}

void Kit::makeSticky()
{
    foreach (KitInformation *ki, KitManager::instance()->kitInformation()) {
        if (hasValue(ki->dataId()))
            makeSticky(ki->dataId());
    }
}

void Kit::makeSticky(Core::Id id)
{
    d->m_sticky.insert(id);
}

void Kit::kitUpdated()
{
    if (d->m_nestedBlockingLevel > 0 && !d->m_mustNotifyAboutDisplayName) {
        d->m_mustNotify = true;
        return;
    }
    validate();
    KitManager::instance()->notifyAboutUpdate(this);
}

void Kit::kitDisplayNameChanged()
{
    if (d->m_nestedBlockingLevel > 0) {
        d->m_mustNotifyAboutDisplayName = true;
        d->m_mustNotify = false;
        return;
    }
    validate();
    KitManager::instance()->notifyAboutDisplayNameChange(this);
}

} // namespace ProjectExplorer
