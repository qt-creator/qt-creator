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

#ifndef QMT_DIAGRAM_MIME_TYPES_H
#define QMT_DIAGRAM_MIME_TYPES_H

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

}

#endif // QMT_DIAGRAM_MIME_TYPES_H
