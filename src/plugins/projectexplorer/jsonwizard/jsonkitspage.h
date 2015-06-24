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

#ifndef JSONKITSPAGE_H
#define JSONKITSPAGE_H

#include "jsonwizard.h"
#include "../targetsetuppage.h"

#include <QVector>

namespace ProjectExplorer {

// Documentation inside.
class JsonKitsPage : public TargetSetupPage
{
    Q_OBJECT

public:
    JsonKitsPage(QWidget *parent = 0);

    void initializePage();
    void cleanupPage();

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

private slots:
    void setupProjectFiles(const JsonWizard::GeneratorFiles &files);

private:
    QString m_unexpandedProjectPath;

    QVector<ConditionalFeature> m_requiredFeatures;
    QVector<ConditionalFeature> m_preferredFeatures;

    Core::FeatureSet evaluate(const QVector<ConditionalFeature> &list, const QVariant &defaultSet,
                              JsonWizard *wiz);
};

} // namespace ProjectExplorer

#endif // JSONKITSPAGE_H
