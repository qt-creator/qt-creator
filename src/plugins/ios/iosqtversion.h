/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
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
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/
#ifndef IOSQTVERSION_H
#define IOSQTVERSION_H

#include <qtsupport/baseqtversion.h>
#include <utils/qtcoverride.h>

#include <QCoreApplication>

namespace Ios {
namespace Internal {

class IosQtVersion : public QtSupport::BaseQtVersion
{
    Q_DECLARE_TR_FUNCTIONS(Ios::Internal::IosQtVersion)

public:
    IosQtVersion();
    IosQtVersion(const Utils::FileName &path, bool isAutodetected = false,
                 const QString &autodetectionSource = QString());

    IosQtVersion *clone() const QTC_OVERRIDE;
    QString type() const QTC_OVERRIDE;
    bool isValid() const QTC_OVERRIDE;
    QString invalidReason() const QTC_OVERRIDE;

    QList<ProjectExplorer::Abi> detectQtAbis() const QTC_OVERRIDE;

    void addToEnvironment(const ProjectExplorer::Kit *k, Utils::Environment &env) const QTC_OVERRIDE;

    Core::FeatureSet availableFeatures() const QTC_OVERRIDE;
    QString platformName() const QTC_OVERRIDE;
    QString platformDisplayName() const QTC_OVERRIDE;

    QString description() const QTC_OVERRIDE;
};

} // namespace Internal
} // namespace Ios

#endif // IOSQTVERSION_H
