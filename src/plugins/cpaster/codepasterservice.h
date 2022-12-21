// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>
#include <QString>

namespace CodePaster {

class Service
{
public:
    virtual ~Service() = default;

    virtual void postText(const QString &text, const QString &mimeType) = 0;
    virtual void postCurrentEditor() = 0;
    virtual void postClipboard() = 0;
};

} // namespace CodePaster

QT_BEGIN_NAMESPACE
Q_DECLARE_INTERFACE(CodePaster::Service, "CodePaster::Service")
QT_END_NAMESPACE
