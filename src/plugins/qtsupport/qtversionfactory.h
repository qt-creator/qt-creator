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

QT_BEGIN_NAMESPACE
class ProFileEvaluator;
QT_END_NAMESPACE

namespace Utils { class FileName; }

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
    virtual BaseQtVersion *create(const Utils::FileName &qmakePath,
                                  ProFileEvaluator *evaluator,
                                  bool isAutoDetected = false,
                                  const QString &autoDetectionSource = QString()) = 0;

    static BaseQtVersion *createQtVersionFromQMakePath(
            const Utils::FileName &qmakePath, bool isAutoDetected = false,
            const QString &autoDetectionSource = QString(), QString *error = nullptr);

protected:
    void setQtVersionCreator(const std::function<BaseQtVersion *()> &creator);
    void setSupportedType(const QString &type);
    void setPriority(int priority);

private:
    std::function<BaseQtVersion *()> m_creator;
    QString m_supportedType;
    int m_priority = 0;
};

} // namespace QtSupport
