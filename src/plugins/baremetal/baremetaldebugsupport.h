// Copyright (C) 2016 Denis Shienkov <denis.shienkov@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/aspects.h>

#include <QtTaskTree/QTaskTree>

#if defined(BAREMETAL_LIBRARY)
#  define BAREMETAL_EXPORT Q_DECL_EXPORT
#else
#  define BAREMETAL_EXPORT Q_DECL_IMPORT
#endif

namespace ProjectExplorer { class RunControl; }

namespace BareMetal {

class BAREMETAL_EXPORT DebugServerProviderAspect final : public Utils::StringAspect
{
    Q_OBJECT

public:
    explicit DebugServerProviderAspect(Utils::AspectContainer *container = nullptr);

private:
    void addToLayoutImpl(Layouting::Layout &parent) final;
};

BAREMETAL_EXPORT QtTaskTree::Group debugServerRecipe(ProjectExplorer::RunControl *runControl,
                                                     const QString &providerId);

namespace Internal {

void setupBareMetalDebugSupport();

} // Internal
} // BareMetal
