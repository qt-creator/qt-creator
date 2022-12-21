// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "sqliteexception.h"

#include <utils/smallstringio.h>

#include <QDebug>

namespace Sqlite {

void ExceptionWithMessage::printWarning() const
{
    qWarning() << what() << m_sqliteErrorMessage;
}

} // namespace Sqlite
