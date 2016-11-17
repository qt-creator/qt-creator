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

#include "operation.h"

#include <QHash>
#include <QString>

class AddKitOperation : public Operation
{
public:
    AddKitOperation();

    QString name() const;
    QString helpText() const;
    QString argumentsHelpText() const;

    bool setArguments(const QStringList &args);

    int execute() const;

#ifdef WITH_TESTS
    bool test() const;
#endif

    static QVariantMap addKit(const QVariantMap &map, const QString &id, const QString &displayName,
                              const QString &icon, const QString &debuggerId,
                              const quint32 &debuggerType, const QString &debugger,
                              const QString &deviceType, const QString &device,
                              const QString &sysRoot, const QHash<QString, QString> &tcs,
                              const QString &qt, const QString &mkspec,
                              const QString &cmakeId, const QString &cmakeGenerator,
                              const QString &cmakeExtraGenerator, const QString &cmakeGeneratorToolset,
                              const QString &cmakeGeneratorPlatform,
                              const QStringList &cmakeConfiguration, const QStringList &env,
                              const KeyValuePairList &extra);

    static QVariantMap initializeKits();

    // internal:
    static QVariantMap addKit(const QVariantMap &map, const QVariantMap &tcMap,
                              const QVariantMap &qtMap, const QVariantMap &devMap, const QVariantMap &cmakeMap,
                              const QString &id, const QString &displayName,
                              const QString &icon, const QString &debuggerId,
                              const quint32 &debuggerType, const QString &debugger,
                              const QString &deviceType, const QString &device,
                              const QString &sysRoot, const QHash<QString, QString> &tcs,
                              const QString &qt, const QString &mkspec, const QString &cmakeId, const QString &cmakeGenerator, const QString &cmakeExtraGenerator, const QString &cmakeGeneratorToolset, const QString &cmakeGeneratorPlatform, const QStringList &cmakeConfiguration, const QStringList &env,
                              const KeyValuePairList &extra);

private:
    QString m_id;
    QString m_displayName;
    QString m_icon;
    QString m_debuggerId;
    quint32 m_debuggerEngine;
    QString m_debugger;
    QString m_deviceType;
    QString m_device;
    QString m_sysRoot;
    QHash<QString, QString> m_tcs;
    QString m_qt;
    QString m_mkspec;
    QString m_cmakeId;
    QString m_cmakeGenerator;
    QString m_cmakeExtraGenerator;
    QString m_cmakeGeneratorToolset;
    QString m_cmakeGeneratorPlatform;
    QStringList m_cmakeConfiguration;
    QStringList m_env;
    KeyValuePairList m_extra;
};
