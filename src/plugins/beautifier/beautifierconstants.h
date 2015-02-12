/**************************************************************************
**
** Copyright (C) 2015 Lorenz Haas
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

#ifndef BEAUTIFIER_BEAUTIFIERCONSTANTS_H
#define BEAUTIFIER_BEAUTIFIERCONSTANTS_H

#include <QtGlobal>

namespace Beautifier {
namespace Constants {

const char ACTION_ID[]              = "Beautifier.Action";
const char MENU_ID[]                = "Beautifier.Menu";
const char OPTION_CATEGORY[]        = "II.Beautifier";
const char OPTION_TR_CATEGORY[]     = QT_TRANSLATE_NOOP("Beautifier", "Beautifier");
const char OPTION_CATEGORY_ICON[]   = ":/beautifier/images/beautifier.png";
const char SETTINGS_GROUP[]         = "Beautifier";
const char SETTINGS_DIRNAME[]       = "beautifier";
const char DOCUMENTATION_DIRNAME[]  = "documentation";
const char DOCUMENTATION_XMLROOT[]  = "beautifier_documentation";
const char DOCUMENTATION_XMLENTRY[] = "entry";
const char DOCUMENTATION_XMLKEYS[]  = "keys";
const char DOCUMENTATION_XMLKEY[]   = "key";
const char DOCUMENTATION_XMLDOC[]   = "doc";

} // namespace Constants
} // namespace Beautifier

#endif // BEAUTIFIER_BEAUTIFIERCONSTANTS_H

