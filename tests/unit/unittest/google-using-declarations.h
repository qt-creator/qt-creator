/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include <gmock/gmock.h>


using testing::_;
using testing::A;
using testing::AllOf;
using testing::An;
using testing::AnyNumber;
using testing::AnyOf;
using testing::Assign;
using testing::ByMove;
using testing::ByRef;
using testing::Contains;
using testing::ContainerEq;
using testing::ElementsAre;
using testing::Field;
using testing::HasSubstr;
using testing::InSequence;
using testing::Invoke;
using testing::IsEmpty;
using testing::IsNull;
using testing::Matcher;
using testing::Mock;
using testing::MockFunction;
using testing::NiceMock;
using testing::NotNull;
using testing::Not;
using testing::Pair;
using testing::PrintToString;
using testing::Property;
using testing::Return;
using testing::ReturnRef;
using testing::SafeMatcherCast;
using testing::Sequence;
using testing::SizeIs;
using testing::StrEq;
using testing::Throw;
using testing::TypedEq;
using testing::UnorderedElementsAre;

using testing::Eq;
using testing::Ge;
using testing::Gt;
using testing::Le;
using testing::Lt;
using testing::Ne;
