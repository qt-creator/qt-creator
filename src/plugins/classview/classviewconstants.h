/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Denis Mingulov.
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef CLASSVIEWCONSTANTS_H
#define CLASSVIEWCONSTANTS_H

namespace ClassView {
namespace Constants {

//! Navi Widget Factory id
const char * const CLASSVIEWNAVIGATION_ID = "Class View";

//! Navi Widget Factory priority
const int CLASSVIEWNAVIGATION_PRIORITY = 500;

//! Settings' group
const char * const CLASSVIEW_SETTINGS_GROUP = "ClassView";

//! Settings' prefix for the tree widget
const char * const CLASSVIEW_SETTINGS_TREEWIDGET_PREFIX = "TreeWidget.";

//! Flat mode settings
const char * const CLASSVIEW_SETTINGS_FLATMODE = "FlatMode";

//! Delay in msecs before an update
const int CLASSVIEW_EDITINGTREEUPDATE_DELAY = 400;

//! QStandardItem roles
enum ItemRole {
    SymbolLocationsRole = Qt::UserRole + 1, //!< Symbol locations
    IconTypeRole,                           //!< Icon type (integer)
    SymbolNameRole,                         //!< Symbol name
    SymbolTypeRole                          //!< Symbol type
};

} // namespace Constants
} // namespace ClassView

#endif // CLASSVIEWCONSTANTS_H
