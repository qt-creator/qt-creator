/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include "qmlprojectmanager_global.h"

#include <qmlprojectmanager/qmlprojectmanagerconstants.h>

#include <projectexplorer/runconfigurationaspects.h>

namespace QmlProjectManager {

class QMLPROJECTMANAGER_EXPORT QmlMultiLanguageAspect : public Utils::BoolAspect
{
    Q_OBJECT
public:
    explicit QmlMultiLanguageAspect(ProjectExplorer::Target *target);
    ~QmlMultiLanguageAspect() override;

    QString currentLocale() const;
    void setCurrentLocale(const QString &locale);
    Utils::FilePath databaseFilePath() const;
    void toMap(QVariantMap &map) const final;
    void fromMap(const QVariantMap &map) final;

    static QmlMultiLanguageAspect *current();
    static QmlMultiLanguageAspect *current(ProjectExplorer::Project *project);
    static QmlMultiLanguageAspect *current(ProjectExplorer::Target *target);

    struct Data : BaseAspect::Data
    {
        const void *origin = nullptr;
    };

signals:
    void currentLocaleChanged(const QString &locale);

private:
    const void *origin() const { return this; }

    ProjectExplorer::Target *m_target = nullptr;
    mutable Utils::FilePath m_databaseFilePath;
    QString m_currentLocale;
};

} // namespace QmlProjectManager
