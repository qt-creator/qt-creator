/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#ifndef QMLJSBUNDLEPROVIDER_H
#define QMLJSBUNDLEPROVIDER_H

#include <QObject>
#include <QHash>

#include "qmljstools_global.h"

namespace ProjectExplorer {
class Kit;
class Project;
class Target;
}

namespace QmlJS {
class QmlLanguageBundles;
class QmlBundle;
} // namespace QmlJS

namespace QmlJSTools {

class QMLJSTOOLS_EXPORT IBundleProvider : public QObject
{
    Q_OBJECT
public:
    explicit IBundleProvider(QObject *parent = 0)
        : QObject(parent)
    { }

    virtual void mergeBundlesForKit(ProjectExplorer::Kit *kit, QmlJS::QmlLanguageBundles &bundles
                                    , const QHash<QString,QString> &replacements) = 0;
};

class QMLJSTOOLS_EXPORT BasicBundleProvider : public IBundleProvider
{
    Q_OBJECT
public:
    explicit BasicBundleProvider(QObject *parent = 0);

    virtual void mergeBundlesForKit(ProjectExplorer::Kit *kit, QmlJS::QmlLanguageBundles &bundles
                                    , const QHash<QString,QString> &replacements);

    static QmlJS::QmlBundle defaultBundle(const QString &bundleInfoName);
    static QmlJS::QmlBundle defaultQt4QtQuick1Bundle();
    static QmlJS::QmlBundle defaultQt5QtQuick1Bundle();
    static QmlJS::QmlBundle defaultQt5QtQuick2Bundle();
    static QmlJS::QmlBundle defaultQbsBundle();
    static QmlJS::QmlBundle defaultQmltypesBundle();
    static QmlJS::QmlBundle defaultQmlprojectBundle();
};

} // end QmlJSTools namespace

#endif // QMLJSBUNDLEPROVIDER_H
