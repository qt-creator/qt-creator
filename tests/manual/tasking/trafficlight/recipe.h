// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef RECIPE_H
#define RECIPE_H

namespace Tasking { class Group; }

class GlueInterface;

Tasking::Group recipe(GlueInterface *iface);

#endif // RECIPE_H
