/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Brian McGillion & Hugues Delorme
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef VCSBASECLIENTSETTINGS_H
#define VCSBASECLIENTSETTINGS_H

#include "vcsbase_global.h"
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVariant>
#include <QtCore/QSharedDataPointer>

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

namespace VCSBase {

namespace Internal {
class VCSBaseClientSettingsPrivate;
} // namespace Internal

class VCSBASE_EXPORT VCSBaseClientSettings
{
public:
    static const QLatin1String binaryPathKey;
    static const QLatin1String userNameKey;
    static const QLatin1String userEmailKey;
    static const QLatin1String logCountKey;
    static const QLatin1String promptOnSubmitKey;
    static const QLatin1String timeoutKey; // Seconds

    VCSBaseClientSettings();
    VCSBaseClientSettings(const VCSBaseClientSettings &other);
    VCSBaseClientSettings &operator=(const VCSBaseClientSettings &other);
    virtual ~VCSBaseClientSettings();

    void writeSettings(QSettings *settings) const;
    void readSettings(const QSettings *settings);

    bool equals(const VCSBaseClientSettings &rhs) const;

    QStringList keys() const;
    bool hasKey(const QString &key) const;

    int *intPointer(const QString &key);
    int intValue(const QString &key, int defaultValue = 0) const;

    bool *boolPointer(const QString &key);
    bool boolValue(const QString &key, bool defaultValue = false) const;

    QString *stringPointer(const QString &key);
    QString stringValue(const QString &key, const QString &defaultValue = QString()) const;

    QVariant value(const QString &key) const;
    void setValue(const QString &key, const QVariant &v);
    QVariant::Type valueType(const QString &key) const;

protected:
    QString settingsGroup() const;
    void setSettingsGroup(const QString &group);

    void declareKey(const QString &key, const QVariant &defaultValue);
    QVariant keyDefaultValue(const QString &key) const;

private:
    friend bool equals(const VCSBaseClientSettings &rhs);
    friend class VCSBaseClientSettingsPrivate;
    QSharedDataPointer<Internal::VCSBaseClientSettingsPrivate> d;
};

inline bool operator==(const VCSBaseClientSettings &s1, const VCSBaseClientSettings &s2)
{ return s1.equals(s2); }
inline bool operator!=(const VCSBaseClientSettings &s1, const VCSBaseClientSettings &s2)
{ return !s1.equals(s2); }

} // namespace VCSBase

#endif // VCSBASECLIENTSETTINGS_H
