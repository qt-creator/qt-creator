// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "testplugin.h"

#include <qplugin.h>

using namespace MyPlugin;

MyPluginImpl::MyPluginImpl()
    : m_isInitialized(false), m_isExtensionsInitialized(false)
{
}

bool MyPluginImpl::initialize(const QStringList &, QString *)
{
    m_isInitialized = true;
    return true;
}

void MyPluginImpl::extensionsInitialized()
{
    m_isExtensionsInitialized = true;
}

