// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <cplusplus/CppDocument.h>

#include <QObject>
#include <QList>

QT_BEGIN_NAMESPACE
class QString;
QT_END_NAMESPACE

namespace CPlusPlus {

class CPLUSPLUS_EXPORT CppModelManagerBase : public QObject
{
    Q_OBJECT
public:
    CppModelManagerBase(QObject *parent = nullptr);
    ~CppModelManagerBase();

    static CppModelManagerBase *instance();
    static bool trySetExtraDiagnostics(const QString &fileName, const QString &kind,
                                       const QList<Document::DiagnosticMessage> &diagnostics);

    virtual bool setExtraDiagnostics(const QString &fileName, const QString &kind,
                                     const QList<Document::DiagnosticMessage> &diagnostics);
    virtual CPlusPlus::Snapshot snapshot() const;
};

} // namespace CPlusPlus
