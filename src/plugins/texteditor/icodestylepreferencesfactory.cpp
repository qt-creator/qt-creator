// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "icodestylepreferencesfactory.h"

#include <QMap>

using namespace Utils;

namespace TextEditor {

static QMap<Id, ICodeStylePreferencesFactory *> g_languageToFactory;

ICodeStylePreferencesFactory::ICodeStylePreferencesFactory(Id languageId)
    : m_languageId(languageId)
{
    if (languageId.isValid())
        g_languageToFactory.insert(languageId, this);
}

ICodeStylePreferencesFactory::~ICodeStylePreferencesFactory()
{
    if (m_languageId.isValid())
        g_languageToFactory.remove(m_languageId);
}

Id ICodeStylePreferencesFactory::languageId() const
{
    return m_languageId;
}

ICodeStylePreferencesFactory *codeStyleFactory(Id languageId)
{
    return g_languageToFactory.value(languageId);
}

const QMap<Id, ICodeStylePreferencesFactory *> &codeStyleFactories()
{
    return g_languageToFactory;
}

} // namespace TextEditor
