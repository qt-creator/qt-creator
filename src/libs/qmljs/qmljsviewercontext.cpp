/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
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

#include "qmljsviewercontext.h"

namespace QmlJS {
/*!
    \class QmlJS::ViewerContext
    \brief The ViewerContext class encapsulate selector and paths for a given viewer.

    Using a a different viewer context can emulate (the pure qml part) of a device.
    This allows checking how a given qml would be interpreted on another platform/viewer.

    Screen information will also most likely need to be added here.
*/
ViewerContext::ViewerContext()
    : language(Dialect::Qml), flags(AddAllPaths)
{ }

ViewerContext::ViewerContext(QStringList selectors, QStringList paths,
                                    QmlJS::Dialect language,
                                    QmlJS::ViewerContext::Flags flags)
    : selectors(selectors), paths(paths), language(language),
      flags(flags)
{ }


/*
 which languages might be imported in this context
 */
bool ViewerContext::languageIsCompatible(Dialect l) const
{
    if (l == Dialect::AnyLanguage && language != Dialect::NoLanguage)
        return true;
    switch (language.dialect()) {
    case Dialect::JavaScript:
    case Dialect::Json:
    case Dialect::QmlProject:
    case Dialect::QmlQbs:
    case Dialect::QmlTypeInfo:
        return language == l;
    case Dialect::Qml:
        return l == Dialect::Qml || l == Dialect::QmlQtQuick1 || l == Dialect::QmlQtQuick2
                || l == Dialect::JavaScript;
    case Dialect::QmlQtQuick1:
        return l == Dialect::Qml || l == Dialect::QmlQtQuick1 || l == Dialect::JavaScript;
    case Dialect::QmlQtQuick2:
    case Dialect::QmlQtQuick2Ui:
        return l == Dialect::Qml || l == Dialect::QmlQtQuick2 || l == Dialect::QmlQtQuick2Ui
            || l == Dialect::JavaScript;
    case Dialect::AnyLanguage:
        return true;
    case Dialect::NoLanguage:
        break;
    }
    return false;
}

void ViewerContext::maybeAddPath(const QString &path)
{
    if (!path.isEmpty() && !paths.contains(path))
        paths.append(path);
}

} // namespace QmlJS
