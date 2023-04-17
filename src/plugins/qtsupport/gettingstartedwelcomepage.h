// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/iwelcomepage.h>
#include <utils/filepath.h>

namespace QtSupport {
namespace Internal {

class ExampleItem;

class ExamplesWelcomePage : public Core::IWelcomePage
{
    Q_OBJECT

public:
    explicit ExamplesWelcomePage(bool showExamples);

    QString title() const final;
    int priority() const final;
    Utils::Id id() const final;
    QWidget *createWidget() const final;

    static void openProject(const ExampleItem *item);

private:
    static Utils::FilePath copyToAlternativeLocation(const Utils::FilePath &fileInfo,
                                                     Utils::FilePaths &filesToOpen,
                                                     const Utils::FilePaths &dependencies);
    const bool m_showExamples;
};

} // namespace Internal
} // namespace QtSupport
