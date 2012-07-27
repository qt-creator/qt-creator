/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (C) 2011 - 2012 Research In Motion
**
** Contact: Research In Motion (blackberry-qt@qnx.com)
** Contact: KDAB (info@kdab.com)
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
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#ifndef QNX_INTERNAL_QNXABSTRACTQTVERSION_H
#define QNX_INTERNAL_QNXABSTRACTQTVERSION_H

#include "qnxconstants.h"

#include <qtsupport/baseqtversion.h>

#include <QCoreApplication>

namespace Qnx {
namespace Internal {

class QnxAbstractQtVersion : public QtSupport::BaseQtVersion
{
    friend class QnxBaseQtConfigWidget;
    Q_DECLARE_TR_FUNCTIONS(Qnx::Internal::QnxAbstractQtVersion)
public:
    QnxAbstractQtVersion();
    QnxAbstractQtVersion(QnxArchitecture arch, const Utils::FileName &path,
                 bool isAutoDetected = false,
                 const QString &autoDetectionSource = QString());

    QString qnxHost() const;
    QString qnxTarget() const;

    QnxArchitecture architecture() const;
    QString archString() const;

    QVariantMap toMap() const;
    void fromMap(const QVariantMap &map);

    QList<ProjectExplorer::Abi> detectQtAbis() const;

    void addToEnvironment(const ProjectExplorer::Profile *p, Utils::Environment &env) const;

    QtSupport::QtConfigWidget *createConfigurationWidget() const;

    bool isValid() const;
    QString invalidReason() const;

    virtual QString sdkDescription() const = 0;

protected:
    QString sdkPath() const;

private:
    void updateEnvironment() const;
    virtual QMultiMap<QString, QString> environment() const = 0;

    void setSdkPath(const QString &sdkPath);

    QnxArchitecture m_arch;
    QString m_sdkPath;

    mutable bool m_environmentUpToDate;
    mutable QMultiMap<QString, QString> m_envMap;
};

} // namespace Internal
} // namespace Qnx

#endif // QNX_INTERNAL_QNXABSTRACTQTVERSION_H
