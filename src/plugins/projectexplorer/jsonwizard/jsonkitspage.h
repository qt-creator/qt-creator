// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "jsonwizard.h"
#include "../targetsetuppage.h"

#include <QVector>

namespace ProjectExplorer {

// Documentation inside.
class JsonKitsPage : public TargetSetupPage
{
    Q_OBJECT

public:
    JsonKitsPage(QWidget *parent = nullptr);

    void initializePage() override;
    void cleanupPage() override;

    void setUnexpandedProjectPath(const QString &path);
    QString unexpandedProjectPath() const;

    void setRequiredFeatures(const QVariant &data);
    void setPreferredFeatures(const QVariant &data);

    class ConditionalFeature {
    public:
        ConditionalFeature() = default;
        ConditionalFeature(const QString &f, const QVariant &c) : feature(f), condition(c)
        { }

        QString feature;
        QVariant condition;
    };
    static QVector<ConditionalFeature> parseFeatures(const QVariant &data,
                                                     QString *errorMessage = nullptr);

private:
    void setupProjectFiles(const JsonWizard::GeneratorFiles &files);

    QString m_unexpandedProjectPath;

    QVector<ConditionalFeature> m_requiredFeatures;
    QVector<ConditionalFeature> m_preferredFeatures;

    QSet<Utils::Id> evaluate(const QVector<ConditionalFeature> &list, const QVariant &defaultSet,
                            JsonWizard *wiz);
};

} // namespace ProjectExplorer
