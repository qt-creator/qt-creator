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
