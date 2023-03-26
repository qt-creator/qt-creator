// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <utils/qtcassert.h>

#include "newdialog.h"

using namespace Core;

NewDialog::NewDialog()
{
    QTC_CHECK(m_currentDialog == nullptr);

    m_currentDialog = this;
}

NewDialog::~NewDialog()
{
    QTC_CHECK(m_currentDialog != nullptr);
    m_currentDialog = nullptr;
}
