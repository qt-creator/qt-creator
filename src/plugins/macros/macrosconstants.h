/**************************************************************************
**
** Copyright (c) 2013 Nicolas Arnaud-Cormos
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef MACROSPLUGIN_MACROSCONSTANTS_H
#define MACROSPLUGIN_MACROSCONSTANTS_H

namespace Macros {
namespace Constants {

const char M_TOOLS_MACRO[] = "Macros.Tools.Menu";

const char START_MACRO[] = "Macros.StartMacro";
const char END_MACRO[] = "Macros.EndMacro";
const char EXECUTE_LAST_MACRO[] = "Macros.ExecuteLastMacro";
const char SAVE_LAST_MACRO[] = "Macros.SaveLastMacro";
const char PREFIX_MACRO[] = "Macros.";

const char M_OPTIONS_PAGE[] = "Macros";
const char M_OPTIONS_TR_PAGE[] = QT_TRANSLATE_NOOP("Macros", "Macros");

const char M_STATUS_BUFFER[] = "Macros.Status";

const char M_EXTENSION[] = "mac";

} // namespace Constants
} // namespace Macros

#endif // MACROSPLUGIN_MACROSCONSTANTS_H
