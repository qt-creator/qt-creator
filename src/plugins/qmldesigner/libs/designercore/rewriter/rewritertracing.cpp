// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "rewritertracing.h"

#include <tracing/qmldesignertracing.h>

#include <sqlitebasestatement.h>

namespace QmlDesigner::RewriterTracing {

#ifdef ENABLE_REWRITER_TRACING

Category &category()
{
    thread_local Category category_{"rewriter",
                                    Tracing::eventQueueWithStringArguments(),
                                    Tracing::eventQueueWithoutArguments(),
                                    category};

    return category_;
}


#endif

} // namespace QmlDesigner::RewriterTracing
