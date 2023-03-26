// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmt/diagram/dannotation.h"
#include "qmt/diagram/dobject.h"
#include "qmt/diagram/dclass.h"

#include "qark/serialize_enum.h"

namespace qark {

QARK_SERIALIZE_ENUM(qmt::DObject::VisualPrimaryRole)
QARK_SERIALIZE_ENUM(qmt::DObject::VisualSecondaryRole)
QARK_SERIALIZE_ENUM(qmt::DObject::StereotypeDisplay)
QARK_SERIALIZE_ENUM(qmt::DClass::TemplateDisplay)
QARK_SERIALIZE_ENUM(qmt::DAnnotation::VisualRole)

} // namespace qark
