// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/uniqueobjectptr.h>

#include <QPointer>

QT_FORWARD_DECLARE_CLASS(QString)
QT_FORWARD_DECLARE_CLASS(QWidget)

namespace QmlDesigner {

class AbstractView;
class BundleImporter;

class BundleHelper
{
public:
    BundleHelper(AbstractView *view, QWidget *widget);
    ~BundleHelper();

    void importBundleToProject();
    QString getImportPath() const;

private:
    void createImporter();
    bool isMaterialBundle(const QString &bundleId) const;
    bool isItemBundle(const QString &bundleId) const;

    QPointer<AbstractView> m_view;
    QPointer<QWidget> m_widget;
    Utils::UniqueObjectPtr<BundleImporter> m_importer;

    static constexpr char BUNDLE_VERSION[] = "1.0";
};

} // namespace QmlDesigner
