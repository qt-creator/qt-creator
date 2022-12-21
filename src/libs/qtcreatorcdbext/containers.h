// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

struct SymbolGroupValueContext;
class AbstractSymbolGroupNode;
class SymbolGroupNode;
class SymbolGroupValue;

#include "common.h"
#include "knowntype.h"

#include <vector>

// Determine size of containers
int containerSize(KnownType kt, const SymbolGroupValue &v);
int containerSize(KnownType kt, SymbolGroupNode *n, const SymbolGroupValueContext &ctx);

/* Create a list of children of containers. */
std::vector<AbstractSymbolGroupNode *> containerChildren(SymbolGroupNode *node,
                                                         int type,
                                                         unsigned size,
                                                         const SymbolGroupValueContext &ctx);
