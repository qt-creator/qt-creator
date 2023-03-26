// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmt/model/melement.h"
#include "qmt/model/mclassmember.h"
#include "qmt/model/mdependency.h"
#include "qmt/model/massociation.h"

#include "qark/serialize_enum.h"

namespace qark {

QARK_SERIALIZE_ENUM(qmt::MClassMember::MemberType)
QARK_SERIALIZE_ENUM(qmt::MClassMember::Property)
QARK_SERIALIZE_ENUM(qmt::MClassMember::Visibility)
QARK_SERIALIZE_ENUM(qmt::MDependency::Direction)
QARK_SERIALIZE_ENUM(qmt::MAssociationEnd::Kind)

} // namespace qark
