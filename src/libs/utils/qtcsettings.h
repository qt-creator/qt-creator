/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "utils_global.h"

#include <QSettings>

namespace Utils {

class QTCREATOR_UTILS_EXPORT QtcSettings : public QSettings
{
public:
    using QSettings::QSettings;

    template<typename T>
    void setValueWithDefault(const QString &key, const T &val, const T &defaultValue);
    template<typename T>
    void setValueWithDefault(const QString &key, const T &val);

    template<typename T>
    static void setValueWithDefault(QSettings *settings,
                                    const QString &key,
                                    const T &val,
                                    const T &defaultValue);
    template<typename T>
    static void setValueWithDefault(QSettings *settings, const QString &key, const T &val);
};

template<typename T>
void QtcSettings::setValueWithDefault(const QString &key, const T &val, const T &defaultValue)
{
    setValueWithDefault(this, key, val, defaultValue);
}

template<typename T>
void QtcSettings::setValueWithDefault(const QString &key, const T &val)
{
    setValueWithDefault(this, key, val);
}

template<typename T>
void QtcSettings::setValueWithDefault(QSettings *settings,
                                      const QString &key,
                                      const T &val,
                                      const T &defaultValue)
{
    if (val == defaultValue)
        settings->remove(key);
    else
        settings->setValue(key, QVariant::fromValue(val));
}

template<typename T>
void QtcSettings::setValueWithDefault(QSettings *settings, const QString &key, const T &val)
{
    if (val == T())
        settings->remove(key);
    else
        settings->setValue(key, QVariant::fromValue(val));
}
} // namespace Utils
