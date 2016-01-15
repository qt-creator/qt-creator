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

#include "cppmodelmanagerbase.h"

namespace CPlusPlus {

static CppModelManagerBase *g_instance = 0;

CppModelManagerBase::CppModelManagerBase(QObject *parent)
    : QObject(parent)
{
    Q_ASSERT(!g_instance);
    g_instance = this;
}

CppModelManagerBase::~CppModelManagerBase()
{
    Q_ASSERT(g_instance == this);
    g_instance = 0;
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
    Q_UNUSED(fileName);
    Q_UNUSED(kind);
    Q_UNUSED(diagnostics);
    return false;
}

CPlusPlus::Snapshot CppModelManagerBase::snapshot() const
{
    return CPlusPlus::Snapshot();
}

} // namespace CPlusPlus
