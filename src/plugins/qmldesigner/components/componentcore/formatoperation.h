// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "selectioncontext.h"

namespace QmlDesigner {
namespace FormatOperation {

struct StylePropertyStruct
{
QString id;
QStringList subclasses;
QStringList properties;
};

struct StyleProperties
{
    QmlDesigner::PropertyName propertyName;
    QVariant value;
};

static QList<StylePropertyStruct> copyableProperties = {};
static QList<StyleProperties> applyableProperties = {};
static StylePropertyStruct chosenItem = {};

bool propertiesCopyable(const SelectionContext &selectionState);
bool propertiesApplyable(const SelectionContext &selectionState);
void copyFormat(const SelectionContext &selectionState);
void applyFormat(const SelectionContext &selectionState);
void readFormatConfiguration();

} // namespace FormatOperation
} // QmlDesigner
