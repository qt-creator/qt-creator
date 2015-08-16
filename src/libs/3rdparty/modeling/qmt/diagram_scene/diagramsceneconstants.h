/***************************************************************************
**
** Copyright (C) 2015 Jochen Becher
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef QMT_DIAGRAMSCENECONSTANTS_H
#define QMT_DIAGRAMSCENECONSTANTS_H

namespace qmt {

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

}

#endif // QMT_DIAGRAMSCENECONSTANTS_H
