/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Copyright (C) 2019 Christoph Schlosser, B/S/H/ <christoph.schlosser@bshg.com>
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

class AddValueOperation : public Operation
{
public:
    QString name() const override;
    QString helpText() const override;
    QString argumentsHelpText() const override;

    bool setArguments(const QStringList &args) override;

    int execute() const override;

#ifdef WITH_TESTS
    bool test() const override;
#endif

    static bool appendListToMap(QVariantMap &map, const QString &key, const QVariantList &values);

private:
    QString m_file;
    QString m_key;
    QVariantList m_values;
};
