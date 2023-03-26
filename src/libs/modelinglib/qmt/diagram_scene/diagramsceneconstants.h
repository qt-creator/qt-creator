// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

namespace qmt {

const int SWIMLANE_ITEMS_ZVALUE = -1100;
const int BOUNDARY_ITEMS_ZVALUE = -1000;
// all model objects have z-values from -500 to 500 depending on their depth in the model tree
const int RELATION_ITEMS_ZVALUE = 1000;
const int RELATION_ITEMS_ZVALUE_SELECTED = 1001;
const int ANNOTATION_ITEMS_ZVALUE = 1500;
const int RELATION_STARTER_ZVALUE = 2000;
const int LATCH_LINES_ZVALUE = 3000;
const int ALIGN_BUTTONS_ZVALUE = 3500;
const int PREVIEW_RELATION_ZVALUE = 4000;

const double RASTER_WIDTH = 5.0;
const double RASTER_HEIGHT = 5.0;

const double CUSTOM_ICON_MINIMUM_AUTO_WIDTH = 40.0; // must be n * 2 * RASTER_WIDTH
const double CUSTOM_ICON_MINIMUM_AUTO_HEIGHT = 40.0; // must be n * 2 * RASTER_HEIGHT

} // namespace qmt
