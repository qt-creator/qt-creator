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

#pragma once

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
