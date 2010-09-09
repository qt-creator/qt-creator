/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Denis Mingulov.
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
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
