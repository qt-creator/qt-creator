// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmlprojectmanager_global.h"

#include <qmlprojectmanager/qmlprojectmanagerconstants.h>

#include <projectexplorer/runconfigurationaspects.h>

namespace QmlProjectManager {

class QMLPROJECTMANAGER_EXPORT QmlMultiLanguageAspect : public Utils::BoolAspect
{
    Q_OBJECT
public:
    explicit QmlMultiLanguageAspect(Utils::AspectContainer *container = nullptr);
    ~QmlMultiLanguageAspect() override;

    void setTarget(ProjectExplorer::Target *target);

    QString currentLocale() const;
    void setCurrentLocale(const QString &locale);
    Utils::FilePath databaseFilePath() const;
    void toMap(Utils::Store &map) const final;
    void fromMap(const Utils::Store &map) final;

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
