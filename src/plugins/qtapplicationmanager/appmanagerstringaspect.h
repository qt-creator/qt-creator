// Copyright (C) 2019 Luxoft Sweden AB
// Copyright (C) 2018 Pelagicore AG
// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/aspects.h>

namespace AppManager::Internal {

class AppManagerIdAspect final : public Utils::StringAspect
{
    Q_OBJECT

public:
    AppManagerIdAspect(Utils::AspectContainer *container = nullptr);

    ~AppManagerIdAspect() final = default;
};

class AppManagerInstanceIdAspect final : public Utils::StringAspect
{
    Q_OBJECT

public:
    AppManagerInstanceIdAspect(Utils::AspectContainer *container = nullptr);

    QString operator()() const;

    struct Data : StringAspect::Data { QString value; };

    ~AppManagerInstanceIdAspect() override = default;
};

class AppManagerDocumentUrlAspect final : public Utils::StringAspect
{
    Q_OBJECT

public:
    AppManagerDocumentUrlAspect(Utils::AspectContainer *container = nullptr);

    ~AppManagerDocumentUrlAspect() final = default;
};

class AppManagerCustomizeAspect final : public Utils::BoolAspect
{
    Q_OBJECT

public:
    AppManagerCustomizeAspect(Utils::AspectContainer *container = nullptr);

    ~AppManagerCustomizeAspect() final = default;
};

class AppManagerRestartIfRunningAspect final : public Utils::BoolAspect
{
    Q_OBJECT

public:
    AppManagerRestartIfRunningAspect(Utils::AspectContainer *container = nullptr);

    ~AppManagerRestartIfRunningAspect() final = default;
};

class AppManagerControllerAspect final : public Utils::FilePathAspect
{
    Q_OBJECT

public:
    AppManagerControllerAspect(Utils::AspectContainer *container = nullptr);

    ~AppManagerControllerAspect() final = default;
};

class AppManagerPackagerAspect final : public Utils::FilePathAspect
{
    Q_OBJECT

public:
    AppManagerPackagerAspect(Utils::AspectContainer *container = nullptr);

    ~AppManagerPackagerAspect() final = default;
};

} // AppManager::Internal
