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

#include "dashboard/concat.h"

#include <utility>

namespace Axivion::Internal::Dto
{

template<typename Input, typename Output>
static Output concat(const std::initializer_list<const Input> &args)
{
    size_t size = 0;
    for (const Input &arg : args)
        size += arg.size();
    Output output;
    output.reserve(size);
    for (const Input &arg : args)
        output += arg;
    return output;
}

std::string concat(const std::initializer_list<const std::string_view> &args)
{
    return concat<std::string_view, std::string>(args);
}

QString concat(const std::initializer_list<const QStringView> &args)
{
    return concat<QStringView, QString>(args);
}

QByteArray concat_bytes(const std::initializer_list<const QByteArrayView> &args)
{
    return concat<QByteArrayView, QByteArray>(args);
}

} // namespace Axivion::Internal::Dto
