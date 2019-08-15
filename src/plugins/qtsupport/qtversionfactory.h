/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "qtsupport_global.h"

#include <QVariantMap>

namespace Utils { class FilePath; }

namespace QtSupport {

class BaseQtVersion;

class QTSUPPORT_EXPORT QtVersionFactory
{
public:
    QtVersionFactory();
    virtual ~QtVersionFactory();

    static const QList<QtVersionFactory *> allQtVersionFactories();

    bool canRestore(const QString &type);
    BaseQtVersion *restore(const QString &type, const QVariantMap &data);

    /// factories with higher priority are asked first to identify
    /// a qtversion, the priority of the desktop factory is 0 and
    /// the desktop factory claims to handle all paths
    int priority() const { return m_priority; }

    static BaseQtVersion *createQtVersionFromQMakePath(
            const Utils::FilePath &qmakePath, bool isAutoDetected = false,
            const QString &autoDetectionSource = QString(), QString *error = nullptr);

protected:
    struct SetupData
    {
        QStringList platforms;
        QStringList config;
        bool isQnx = false; // eeks...
    };

    void setQtVersionCreator(const std::function<BaseQtVersion *()> &creator);
    void setRestrictionChecker(const std::function<bool(const SetupData &)> &checker);
    void setSupportedType(const QString &type);
    void setPriority(int priority);

private:
    friend class BaseQtVersion;
    BaseQtVersion *create() const;

    std::function<BaseQtVersion *()> m_creator;
    std::function<bool(const SetupData &)> m_restrictionChecker;
    QString m_supportedType;
    int m_priority = 0;
};

} // namespace QtSupport
