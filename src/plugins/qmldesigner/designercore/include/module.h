// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <modelfwd.h>
#include <projectstorageids.h>

namespace QmlDesigner {

class Module
{
public:
    Module() = default;

    Module(ModuleId moduleId, NotNullPointer<const ProjectStorageType> projectStorage)
        : m_projectStorage{projectStorage}
        , m_id{moduleId}
    {}

    ModuleId id() const { return m_id; }

    bool isValid() const { return bool(m_id); }

    explicit operator bool() const { return isValid(); }

    const ProjectStorageType &projectStorage() const { return *m_projectStorage; }

    friend bool operator==(Module first, Module second) { return first.m_id == second.m_id; }

private:
    NotNullPointer<const ProjectStorageType> m_projectStorage = {};
    ModuleId m_id;
};
} // namespace QmlDesigner
