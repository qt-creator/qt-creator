// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/environment.h>

#include <QProcessEnvironment>

namespace ProjectExplorer {
class Target;
}

namespace QmlDesigner {

enum class PuppetType { Fallback, UserSpace, Kit };

class PuppetEnvironmentBuilder
{
public:
    PuppetEnvironmentBuilder(ProjectExplorer::Target *target,
                             const class DesignerSettings &designerSettings,
                             const class Model &model,
                             const Utils::FilePath &qmlPuppetPath)
        : m_target(target)
        , m_designerSettings(designerSettings)
        , m_model(model)
        , m_qmlPuppetPath(qmlPuppetPath)
    {}

    QProcessEnvironment processEnvironment() const;

    static QProcessEnvironment createEnvironment(ProjectExplorer::Target *target,
                                                 const class DesignerSettings &designerSettings,
                                                 const class Model &model,
                                                 const Utils::FilePath &qmlPuppetPath);

private:
    PuppetType determinePuppetType() const;
    bool usesVirtualKeyboard() const;
    QString getStyleConfigFileName() const;
    void addKit() const;
    void addRendering() const;
    void addControls() const;
    void addPixelRatio() const;
    void addVirtualKeyboard() const;
    void addQuick3D() const;
    void addForceQApplication() const;
    void addMultiLanguageDatatbase() const;
    void addImportPaths() const;
    void addCustomFileSelectors() const;
    void addDisableDeferredProperties() const;
    void addResolveUrlsOnAssignment() const;

private:
    ProjectExplorer::Target *m_target = nullptr;
    const DesignerSettings &m_designerSettings;
    const Model &m_model;
    mutable PuppetType m_availablePuppetType = {};
    mutable Utils::Environment m_environment;
    Utils::FilePath m_qmlPuppetPath;
};

} // namespace QmlDesigner
