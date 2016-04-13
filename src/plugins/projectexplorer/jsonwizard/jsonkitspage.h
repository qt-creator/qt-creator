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
                                                     QString *errorMessage = 0);

private:
    void setupProjectFiles(const JsonWizard::GeneratorFiles &files);

    QString m_unexpandedProjectPath;

    QVector<ConditionalFeature> m_requiredFeatures;
    QVector<ConditionalFeature> m_preferredFeatures;

    QSet<Core::Id> evaluate(const QVector<ConditionalFeature> &list, const QVariant &defaultSet,
                            JsonWizard *wiz);
};

} // namespace ProjectExplorer
