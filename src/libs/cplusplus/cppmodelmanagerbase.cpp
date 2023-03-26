// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cppmodelmanagerbase.h"

namespace CPlusPlus {

static CppModelManagerBase *g_instance = nullptr;

CppModelManagerBase::CppModelManagerBase(QObject *parent)
    : QObject(parent)
{
    Q_ASSERT(!g_instance);
    g_instance = this;
}

CppModelManagerBase::~CppModelManagerBase()
{
    Q_ASSERT(g_instance == this);
    g_instance = nullptr;
}

CppModelManagerBase *CppModelManagerBase::instance()
{
    return g_instance;
}

bool CppModelManagerBase::trySetExtraDiagnostics(const QString &fileName, const QString &kind,
                                                 const QList<CPlusPlus::Document::DiagnosticMessage> &diagnostics)
{
    if (CppModelManagerBase *mm = instance())
        return mm->setExtraDiagnostics(fileName, kind, diagnostics);
    return false;
}

bool CppModelManagerBase::setExtraDiagnostics(const QString &fileName, const QString &kind,
                                              const QList<CPlusPlus::Document::DiagnosticMessage> &diagnostics)
{
    Q_UNUSED(fileName)
    Q_UNUSED(kind)
    Q_UNUSED(diagnostics)
    return false;
}

CPlusPlus::Snapshot CppModelManagerBase::snapshot() const
{
    return CPlusPlus::Snapshot();
}

} // namespace CPlusPlus
