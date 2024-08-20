// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QVariant>

namespace QmlDesigner {
class MetaInfo;

namespace Internal {
namespace PropertyParser {

QVariant read(const QString &typeStr, const QString &str, const MetaInfo &metaInfo);
QVariant read(const QString &typeStr, const QString &str);
QVariant read(int variantType, const QString &str);
QVariant variantFromString(const QString &s);

} // namespace PropertyParser
} // namespace Internal
} // namespace Designer
