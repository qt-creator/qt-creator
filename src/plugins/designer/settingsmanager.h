/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#ifndef SETTINGSMANAGER_H
#define SETTINGSMANAGER_H

#include <qt_private/abstractsettings_p.h>
#include <QtCore/QSettings>

namespace Designer {
namespace Internal {

/* Prepends "Designer" to every value stored/retrieved by designer plugins,
   to avoid namespace polution. We cannot use a group because groups cannot be nested,
   and designer uses groups internally. */
class SettingsManager : public QDesignerSettingsInterface
{
public:
    virtual void beginGroup(const QString &prefix);
    virtual void endGroup();

    virtual bool contains(const QString &key) const;
    virtual void setValue(const QString &key, const QVariant &value);
    virtual QVariant value(const QString &key, const QVariant &defaultValue = QVariant()) const ;
    virtual void remove(const QString &key);

private:
    QString addPrefix(const QString &name) const;
    QSettings m_settings;
};

} // namespace Internal
} // namespace Designer

#endif // SETTINGSMANAGER_H
