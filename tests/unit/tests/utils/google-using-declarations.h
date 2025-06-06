// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <gmock/gmock.h>
#include <matchers/property-matcher.h>

using testing::_;
using testing::A;
using testing::AllOf;
using testing::An;
using testing::AnyNumber;
using testing::AnyOf;
using testing::Assign;
using testing::AtLeast;
using testing::AtMost;
using testing::Between;
using testing::ByMove;
using testing::ByRef;
using testing::Conditional;
using testing::ContainerEq;
using testing::Contains;
using testing::Each;
using testing::ElementsAre;
using testing::Eq;
using testing::Exactly;
using testing::Field;
using testing::FieldsAre;
using testing::Ge;
using testing::Gt;
using testing::HasSubstr;
using testing::InSequence;
using testing::Invoke;
using testing::IsFalse;
using testing::IsSubsetOf;
using testing::IsSupersetOf;
using testing::IsTrue;
using testing::Le;
using testing::Lt;
using testing::Matcher;
using testing::Mock;
using testing::MockFunction;
using testing::Ne;
using testing::NiceMock;
using testing::Not;
using testing::NotNull;
using testing::Optional;
using testing::Pair;
using testing::PrintToString;
using testing::Property;
using testing::Return;
using testing::ReturnArg;
using testing::ReturnRef;
using testing::SafeMatcherCast;
using testing::SaveArg;
using testing::SaveArgPointee;
using testing::Sequence;
using testing::SizeIs;
using testing::StrEq;
using testing::Throw;
using testing::TypedEq;
using testing::UnorderedElementsAre;
using testing::UnorderedElementsAreArray;
using testing::VariantWith;
using testing::WithArg;
