// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/aspects.h>
#include <utils/filepath.h>

#include <QPointer>

QT_BEGIN_NAMESPACE
class QTreeView;
QT_END_NAMESPACE

namespace QmlJSEditor::Internal {

// Custom static analyzer message selection, edited through a two-column
// checkable tree. Persisted under the legacy DISABLED_MESSAGES /
// DISABLED_MESSAGES_NONQUICKUI keys, which QmlJS::Check reads directly.
class AnalyzerMessagesAspect final : public Utils::BaseAspect
{
public:
    AnalyzerMessagesAspect(Utils::AspectContainer *container = nullptr);

    void apply() final;
    void cancel() final;
    bool isDirty() const final;
    void readSettings() final;
    void writeSettings() const final;

    void addToLayoutImpl(Layouting::Layout &parent) final;

private:
    void populateModel();

    QList<int> m_disabled;
    QList<int> m_disabledForNonQuickUi;
    QPointer<QTreeView> m_view = nullptr;
};

// UI-only aspect (nothing persisted): hosts the "Install Qt Design Studio"
// button and keeps the qdsCommand placeholder in sync. All behavior lives in
// addToLayoutImpl.
class QdsInstallAspect final : public Utils::BaseAspect
{
public:
    QdsInstallAspect(Utils::AspectContainer *container = nullptr);

    void addToLayoutImpl(Layouting::Layout &parent) final;
};

class QmlJsEditingSettings final : public Utils::AspectContainer
{
public:
    QmlJsEditingSettings();

    Utils::FilePath defaultQdsCommand() const;

    Utils::BoolAspect enableContextPane{this};
    Utils::BoolAspect pinContextPane{this};
    Utils::BoolAspect autoFormatOnSave{this};
    Utils::BoolAspect autoFormatOnlyCurrentProject{this};
    Utils::BoolAspect foldAuxData{this};
    Utils::BoolAspect useCustomAnalyzer{this};
    Utils::SelectionAspect uiQmlOpenMode{this};
    AnalyzerMessagesAspect analyzerMessages{this};
    Utils::FilePathAspect qdsCommand{this};
    QdsInstallAspect qdsInstall{this};
};

QmlJsEditingSettings &settings();

void setupQmlJsEditingSettings();

} // QmlJSEditor::Internal
