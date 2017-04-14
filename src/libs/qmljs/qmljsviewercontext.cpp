/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
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

ViewerContext::ViewerContext(const QStringList &selectors, const QStringList &paths,
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
