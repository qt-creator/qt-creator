// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "aspects.h"
#include "environment.h"

namespace Utils {

class QTCREATOR_UTILS_EXPORT EnvironmentChangesAspect
    : public TypedAspect<EnvironmentChanges>
{
    using TypedAspect::TypedAspect;

private:
    QVariant variantValue() const override { return m_value.toVariant(); }
    QVariant volatileVariantValue() const override { return m_volatileValue.toVariant(); }
    QVariant defaultVariantValue() const override { return m_default.toVariant(); }

    void setVariantValue(const QVariant &value, Announcement howToAnnounce = DoEmit) override
    {
        setValue(EnvironmentChanges::createFromVariant(value), howToAnnounce);
    }

    void setDefaultVariantValue(const QVariant &value) override
    {
        setDefaultValue(EnvironmentChanges::createFromVariant(value));
    }

    void addToLayoutImpl(Layouting::Layout &parent) override;
};

} // namespace Utils
