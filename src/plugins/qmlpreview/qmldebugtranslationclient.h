/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#pragma once

#include "qmlpreview_global.h"
#include <qmldebug/qmldebugclient.h>

namespace QmlPreview {

class QMLPREVIEW_EXPORT QmlDebugTranslationClient : public QmlDebug::QmlDebugClient
{
    Q_OBJECT
public:
    //needs to be in sync with QQmlDebugTranslationClient in qtdeclarative/src/plugins/qmltooling/qmldbg_preview/qqmldebugtranslationservice.h
    enum Command {
        ChangeLanguage,
        ChangeWarningColor,
        ChangeElidedTextWarningString,
        SetDebugTranslationServiceLogFile,
        EnableElidedTextWarning,
        DisableElidedTextWarning,
        TestAllLanguages
    };

    explicit QmlDebugTranslationClient(QmlDebug::QmlDebugConnection *connection);

    void changeLanguage(const QUrl &url, const QString &locale);
    void changeWarningColor(const QColor &warningColor);
    void changeElidedTextWarningString(const QString &warningString); //is QByteArray better here?
    void setDebugTranslationServiceLogFile(const QString &logFilePath);
    void enableElidedTextWarning();
    void disableElidedTextWarning();

    void messageReceived(const QByteArray &message) override;
    void stateChanged(State state) override;

signals:
//    void pathRequested(const QString &path);
//    void errorReported(const QString &error);
    void debugServiceUnavailable();
};

} // namespace QmlPreview
