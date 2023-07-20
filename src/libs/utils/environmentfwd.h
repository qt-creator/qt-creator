// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QList>

QT_BEGIN_NAMESPACE
class QTreeView;
QT_END_NAMESPACE

namespace Utils {
class NameValueDictionary;
class NameValueItem;
using NameValueItems = QList<NameValueItem>;

class Environment;
using EnvironmentItem = NameValueItem;
using EnvironmentItems = NameValueItems;

class PreprocessorMacroDictionary;
using PreprocessorMacroItem = NameValueItem;
using PreprocessorMacroItems = NameValueItems;

class NameValueModel;
class EnvironmentModel;
} // namespace Utils
