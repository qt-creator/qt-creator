// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmljseditor_global.h"

#include <utils/fileutils.h>

#include <languageclient/client.h>
#include <languageclient/languageclientinterface.h>

namespace QmlJSEditor {

class QMLJSEDITOR_EXPORT QmllsClient : public LanguageClient::Client
{
    Q_OBJECT
public:
    explicit QmllsClient(LanguageClient::StdIOClientInterface *interface);
    ~QmllsClient();

    static QmllsClient *clientForQmlls(const Utils::FilePath &qmlls);
};

} // namespace QmlJSEditor
