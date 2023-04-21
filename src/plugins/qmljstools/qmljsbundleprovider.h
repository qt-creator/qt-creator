// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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

namespace QtSupport {
class QtVersion;
}

namespace QmlJSTools {

class QMLJSTOOLS_EXPORT IBundleProvider : public QObject
{
    Q_OBJECT
public:
    explicit IBundleProvider(QObject *parent = nullptr);
    ~IBundleProvider() override;

    static const QList<IBundleProvider *> allBundleProviders();

    virtual void mergeBundlesForKit(ProjectExplorer::Kit *kit, QmlJS::QmlLanguageBundles &bundles
                                    , const QHash<QString,QString> &replacements) = 0;
};

class QMLJSTOOLS_EXPORT BasicBundleProvider : public IBundleProvider
{
    Q_OBJECT
public:
    explicit BasicBundleProvider(QObject *parent = nullptr);

    void mergeBundlesForKit(ProjectExplorer::Kit *kit, QmlJS::QmlLanguageBundles &bundles,
                            const QHash<QString,QString> &replacements) override;

    static QmlJS::QmlBundle defaultBundle(const QString &bundleInfoName,
                                          QtSupport::QtVersion *qtVersion);
    static QmlJS::QmlBundle defaultQt5QtQuick2Bundle(QtSupport::QtVersion *qtVersion);
    static QmlJS::QmlBundle defaultQbsBundle();
    static QmlJS::QmlBundle defaultQmltypesBundle();
    static QmlJS::QmlBundle defaultQmlprojectBundle();
};

} // end QmlJSTools namespace
