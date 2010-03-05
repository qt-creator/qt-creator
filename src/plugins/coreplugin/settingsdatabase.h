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

#ifndef SETTINGSDATABASE_H
#define SETTINGSDATABASE_H

#include "core_global.h"

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVariant>

namespace Core {

namespace Internal {
class SettingsDatabasePrivate;
}

class CORE_EXPORT SettingsDatabase : public QObject
{
public:
    SettingsDatabase(const QString &path, const QString &application, QObject *parent = 0);
    ~SettingsDatabase();

    void setValue(const QString &key, const QVariant &value);
    QVariant value(const QString &key, const QVariant &defaultValue = QVariant()) const;
    bool contains(const QString &key) const;
    void remove(const QString &key);

    void beginGroup(const QString &prefix);
    void endGroup();
    QString group() const;
    QStringList childKeys() const;

    void sync();

private:
    Internal::SettingsDatabasePrivate *d;
};

} // namespace Core

#endif // SETTINGSDATABASE_H
