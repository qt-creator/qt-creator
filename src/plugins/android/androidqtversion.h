/**************************************************************************
**
** Copyright (C) 2015 BogDan Vatra <bog_dan_ro@yahoo.com>
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

#ifndef ANDROIDQTVERSION_H
#define ANDROIDQTVERSION_H

#include <qtsupport/baseqtversion.h>

#include <QCoreApplication>

namespace Android {
namespace Internal {

class AndroidQtVersion : public QtSupport::BaseQtVersion
{
    Q_DECLARE_TR_FUNCTIONS(Android::Internal::AndroidQtVersion)

public:
    AndroidQtVersion();
    AndroidQtVersion(const Utils::FileName &path, bool isAutodetected = false, const QString &autodetectionSource = QString());

    AndroidQtVersion *clone() const;
    QString type() const;
    bool isValid() const;
    QString invalidReason() const;

    QList<ProjectExplorer::Abi> detectQtAbis() const;

    void addToEnvironment(const ProjectExplorer::Kit *k, Utils::Environment &env) const;
    Utils::Environment qmakeRunEnvironment() const;

    Core::FeatureSet availableFeatures() const;
    QString platformName() const;
    QString platformDisplayName() const;

    QString description() const;
    QString targetArch() const;
protected:
    virtual void parseMkSpec(ProFileEvaluator *) const;
private:
    mutable QString m_targetArch;
};

} // namespace Internal
} // namespace Android

#endif // ANDROIDQTVERSION_H
