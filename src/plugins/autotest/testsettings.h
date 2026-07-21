// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/aspects.h>

#include <QTreeWidget>

namespace Utils { class InfoLabel; }

namespace Autotest::Internal {

enum class RunAfterBuildMode
{
    None,
    All,
    Selected
};

class FrameworksAspect : public Utils::BaseAspect
{
public:
    explicit FrameworksAspect(Utils::AspectContainer *container);

    bool framework(Utils::Id id) const;
    bool frameworkGrouping(Utils::Id id) const;
    bool tool(Utils::Id id) const;

    struct Data
    {
        QHash<Utils::Id, bool> frameworks;
        QHash<Utils::Id, bool> frameworksGrouping;
        QHash<Utils::Id, bool> tools;
    };

private:
    void apply() final;
    void cancel() final;
    bool isDirty() const final;

    void writeSettings() const final;
    void readSettings() final;

    void addToLayoutImpl(Layouting::Layout &parent) final;

    void populateTreeWidget();
    void onFrameworkItemChanged();
    void updateWarning(bool init);
    bool showWarning();

    QTreeWidget *m_frameworkTreeWidget = nullptr;
    Utils::InfoLabel *m_frameworksWarn = nullptr;
    Data m_data;
};

class TestSettings : public Utils::AspectContainer
{
public:
    TestSettings();

    Utils::IntegerAspect scanThreadLimit{this};
    Utils::BoolAspect useTimeout{this};
    Utils::IntegerAspect timeout{this};
    Utils::BoolAspect omitInternalMsg{this};
    Utils::BoolAspect omitRunConfigWarn{this};
    Utils::BoolAspect limitResultOutput{this};
    Utils::BoolAspect limitResultDescription{this};
    Utils::IntegerAspect resultDescriptionMaxSize{this};
    Utils::BoolAspect autoScroll{this};
    Utils::BoolAspect processArgs{this};
    Utils::BoolAspect displayApplication{this};
    Utils::BoolAspect popupOnStart{this};
    Utils::BoolAspect popupOnFinish{this};
    Utils::BoolAspect popupOnFail{this};
    Utils::BoolAspect showTreeFilterTextInput{this};
    Utils::SelectionAspect runAfterBuild{this};
    FrameworksAspect frameworks{this};

    RunAfterBuildMode runAfterBuildMode() const;
};

TestSettings &testSettings();

void setupTestSettings();

} // Autotest::Internal
