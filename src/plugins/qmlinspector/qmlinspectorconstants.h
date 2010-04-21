/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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
#ifndef QMLINSPECTORCONSTANTS_H
#define QMLINSPECTORCONSTANTS_H

#include <QString>

namespace Qml {
    namespace Constants {
        const char * const RUN = "QmlInspector.Run";
        const char * const STOP = "QmlInspector.Stop";

        const char * const C_INSPECTOR = "QmlInspector";
        const char * const COMPLETE_THIS = "QmlInspector.CompleteThis";

        const char * const M_DEBUG_SIMULTANEOUSLY = "QmlInspector.Menu.SimultaneousDebug";

        const char * const LANG_QML = "QML";

        // settings
        const char * const S_QML_INSPECTOR    = "QML.Inspector";
        const char * const S_EXTERNALPORT_KEY = "ExternalPort";
        const char * const S_EXTERNALURL_KEY  = "ExternalUrl";
        const char * const S_SHOW_UNINSPECTABLE_ITEMS  = "ShowUninspectableProperties";
        const char * const S_SHOW_UNWATCHABLE_PROPERTIES = "ShowUninspectableItem";
        const char * const S_GROUP_PROPERTIES_BY_ITEM_TYPE = "GroupPropertiesByItemType";

    };

};

#endif
