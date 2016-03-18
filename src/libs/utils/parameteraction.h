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

#include "utils_global.h"

#include <QAction>

namespace Utils {

class QTCREATOR_UTILS_EXPORT ParameterAction : public QAction
{
    Q_ENUMS(EnablingMode)
    Q_PROPERTY(QString emptyText READ emptyText WRITE setEmptyText)
    Q_PROPERTY(QString parameterText READ parameterText WRITE setParameterText)
    Q_PROPERTY(EnablingMode enablingMode READ enablingMode WRITE setEnablingMode)
    Q_OBJECT
public:
    enum EnablingMode { AlwaysEnabled, EnabledWithParameter };

    explicit ParameterAction(const QString &emptyText,
                             const QString &parameterText,
                             EnablingMode em = AlwaysEnabled,
                             QObject *parent = 0);

    QString emptyText() const;
    void setEmptyText(const QString &);

    QString parameterText() const;
    void setParameterText(const QString &);

    EnablingMode enablingMode() const;
    void setEnablingMode(EnablingMode m);

public slots:
    void setParameter(const QString &);

private:
    QString m_emptyText;
    QString m_parameterText;
    EnablingMode m_enablingMode;
};

} // namespace Utils
