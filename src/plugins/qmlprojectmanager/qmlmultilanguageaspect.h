// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmlprojectmanager_global.h"

#include <utils/aspects.h>
#include <utils/filepath.h>

#include <QString>

namespace ProjectExplorer {
class Project;
class Target;
}

namespace QmlProjectManager {

class QMLPROJECTMANAGER_EXPORT QmlMultiLanguageAspect : public Utils::BoolAspect
{
    Q_OBJECT
public:
    explicit QmlMultiLanguageAspect(Utils::AspectContainer *container = nullptr);

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

private:
    const void *origin() const { return this; }

    QString m_currentLocale;
};

} // namespace QmlProjectManager
