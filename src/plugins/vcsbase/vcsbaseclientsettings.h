/**************************************************************************
**
** Copyright (C) 2015 Brian McGillion and Hugues Delorme
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef VCSBASECLIENTSETTINGS_H
#define VCSBASECLIENTSETTINGS_H

#include "vcsbase_global.h"

#include <QStringList>
#include <QVariant>
#include <QSharedDataPointer>

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

namespace Utils { class FileName; }
namespace VcsBase {

namespace Internal { class VcsBaseClientSettingsPrivate; }

class VCSBASE_EXPORT VcsBaseClientSettings
{
public:
    static const QLatin1String binaryPathKey;
    static const QLatin1String userNameKey;
    static const QLatin1String userEmailKey;
    static const QLatin1String logCountKey;
    static const QLatin1String promptOnSubmitKey;
    static const QLatin1String timeoutKey; // Seconds
    static const QLatin1String pathKey;

    VcsBaseClientSettings();
    VcsBaseClientSettings(const VcsBaseClientSettings &other);
    VcsBaseClientSettings &operator=(const VcsBaseClientSettings &other);
    virtual ~VcsBaseClientSettings();

    void writeSettings(QSettings *settings) const;
    void readSettings(const QSettings *settings);

    bool equals(const VcsBaseClientSettings &rhs) const;

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

    Utils::FileName binaryPath() const;

    QStringList searchPathList() const;

protected:
    QString settingsGroup() const;
    void setSettingsGroup(const QString &group);

    void declareKey(const QString &key, const QVariant &defaultValue);
    QVariant keyDefaultValue(const QString &key) const;

    virtual void readLegacySettings(const QSettings *settings);

private:
    friend bool equals(const VcsBaseClientSettings &rhs);
    friend class VcsBaseClientSettingsPrivate;
    QSharedDataPointer<Internal::VcsBaseClientSettingsPrivate> d;
};

inline bool operator==(const VcsBaseClientSettings &s1, const VcsBaseClientSettings &s2)
{ return s1.equals(s2); }
inline bool operator!=(const VcsBaseClientSettings &s1, const VcsBaseClientSettings &s2)
{ return !s1.equals(s2); }

} // namespace VcsBase

#endif // VCSBASECLIENTSETTINGS_H
