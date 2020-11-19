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

#include "qtcsettings.h"

namespace Utils {

/*!
    \class Utils::QtcSettings
    \inheaderfile utils/qtcsettings.h
    \inmodule QtCreator

    \brief The QtcSettings class is an extension of the QSettings class.

    Use Utils::QtcSettings::setValueWithDefault() to write values with a
    default.
*/

/*!
    \fn template<typename T> void setValueWithDefault(const QString &key, const T &val, const T &defaultValue)

    Sets the value of setting \a key to \a val. If \a val is the same as the \a
    defaultValue, the settings key is removed instead. This makes sure that
    settings are only written if actually necessary, namely when the user
    changed them from the default. It also makes a new default value for a
    setting in a new version of the application take effect, if the user did
    not change the setting before.

    \sa QSettings::setValue()
*/

} // namespace Utils
