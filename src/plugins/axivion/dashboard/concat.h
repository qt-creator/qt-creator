#pragma once

/*
 * Copyright (C) 2022-current by Axivion GmbH
 * https://www.axivion.com/
 *
 * SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
 *
 * Purpose: Helper functions to concatenate strings/bytes
 *
 * !!!!!! GENERATED, DO NOT EDIT !!!!!!
 *
 * This file was generated with the script at
 * <AxivionSuiteRepo>/projects/libs/dashboard_cpp_api/generator/generate_dashboard_cpp_api.py
 */

#include <QByteArray>
#include <QByteArrayView>
#include <QString>
#include <QStringView>

#include <string>
#include <string_view>

namespace Axivion::Internal::Dto
{
std::string concat(const std::initializer_list<const std::string_view> &args);

QString concat(const std::initializer_list<const QStringView> &args);

QByteArray concat_bytes(const std::initializer_list<const QByteArrayView> &args);

} // namespace Axivion::Internal::Dto
