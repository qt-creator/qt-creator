// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

namespace qmt {

// mime types for drag&drop
static const char MIME_TYPE_MODEL_ELEMENTS[] = "text/model-elements";
static const char MIME_TYPE_NEW_MODEL_ELEMENTS[] = "text/new-model-elements";

// object type constants used within drag&drop
static const char ELEMENT_TYPE_PACKAGE[] = "package";
static const char ELEMENT_TYPE_COMPONENT[] = "component";
static const char ELEMENT_TYPE_CLASS[] = "class";
static const char ELEMENT_TYPE_ITEM[] = "item";
static const char ELEMENT_TYPE_ANNOTATION[] = "annotation";
static const char ELEMENT_TYPE_BOUNDARY[] = "boundary";
static const char ELEMENT_TYPE_SWIMLANE[] = "swimlane";

} // namespace qmt
