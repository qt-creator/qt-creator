// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmt_global.h"

#include <QString>
#include <QCoreApplication>

namespace qmt {

class QMT_EXPORT Exception
{
public:
    explicit Exception(const QString &errorMessage);
    virtual ~Exception() = default;

    QString errorMessage() const { return m_errorMessage; }
    void setErrorMessage(const QString &errorMessage) { m_errorMessage = errorMessage; }

private:
    QString m_errorMessage;
};

class NullPointerException : public Exception
{
public:
    NullPointerException();
};

} // namespace qmt
