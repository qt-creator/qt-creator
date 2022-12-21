// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "idocumentfactory.h"

#include <utils/qtcassert.h>

namespace Core {

static QList<IDocumentFactory *> g_documentFactories;

IDocumentFactory::IDocumentFactory()
{
    g_documentFactories.append(this);
}

IDocumentFactory::~IDocumentFactory()
{
    g_documentFactories.removeOne(this);
}

const QList<IDocumentFactory *> IDocumentFactory::allDocumentFactories()
{
    return g_documentFactories;
}

IDocument *IDocumentFactory::open(const Utils::FilePath &filePath)
{
    QTC_ASSERT(m_opener, return nullptr);
    return m_opener(filePath);
}

} // namespace Core
