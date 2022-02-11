/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#include <QObject>

namespace Utils {
class FilePath;
} // namespace Utils

namespace McuSupport::Internal {

class McuAbstractPackage : public QObject
{
    Q_OBJECT
public:
    enum Status {
        EmptyPath,
        InvalidPath,
        ValidPathInvalidPackage,
        ValidPackageMismatchedVersion,
        ValidPackage
    };

    virtual Utils::FilePath basePath() const = 0;
    virtual Utils::FilePath path() const = 0;
    virtual QString label() const = 0;
    virtual Utils::FilePath defaultPath() const = 0;
    virtual QString detectionPath() const = 0;
    virtual QString statusText() const = 0;
    virtual void updateStatus() = 0;

    virtual Status status() const = 0;
    virtual bool validStatus() const = 0;
    virtual const QString &environmentVariableName() const = 0;
    virtual void setAddToPath(bool) = 0;
    virtual bool addToPath() const = 0;
    virtual void writeGeneralSettings() const = 0;
    virtual bool writeToSettings() const = 0;
    virtual void setRelativePathModifier(const QString &) = 0;
    virtual void setVersions(const QStringList &) = 0;

    virtual bool automaticKitCreationEnabled() const = 0;
    virtual void setAutomaticKitCreationEnabled(const bool enabled) = 0;

    virtual QWidget *widget() = 0;
signals:
    void changed();
    void statusChanged();

}; // class McuAbstractPackage
} // namespace McuSupport::Internal
