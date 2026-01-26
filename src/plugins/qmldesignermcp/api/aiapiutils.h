// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QString>

QT_FORWARD_DECLARE_CLASS(QImage)

namespace QmlDesigner {

class AiResponse;

namespace AiApiUtils {

QString toBase64Image(const QImage &image, const QByteArray &format);

AiResponse aiResponseFromContent(const QString &contentStr);

} // namespace AiApiUtils

} // namespace QmlDesigner
