// Copyright (C) 2019 Luxoft Sweden AB
// Copyright (C) 2018 Pelagicore AG
// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "appmanagerstringaspect.h"

#include <utils/algorithm.h>
#include <utils/layoutbuilder.h>

#include <QFileInfo>
#include <QLayout>
#include <QPushButton>

using namespace Utils;

namespace AppManager {
namespace Internal {

AppManagerIdAspect::AppManagerIdAspect(Utils::AspectContainer *container)
    : StringAspect(container)
{
    setSettingsKey("ApplicationManagerPlugin.ApplicationId");
    setDisplayStyle(StringAspect::LineEditDisplay);
    setLabelText(Tr::tr("Application Id:"));
    //        setReadOnly(true);
}

AppManagerInstanceIdAspect::AppManagerInstanceIdAspect(Utils::AspectContainer *container)
    : StringAspect(container)
{
    setSettingsKey("ApplicationManagerPlugin.InstanceId");
    setDisplayStyle(StringAspect::LineEditDisplay);
    setLabelText(Tr::tr("AppMan Instance Id:"));
}

AppManagerDocumentUrlAspect::AppManagerDocumentUrlAspect(Utils::AspectContainer *container)
    : StringAspect(container)
{
    setSettingsKey("ApplicationManagerPlugin.DocumentUrl");
    setDisplayStyle(StringAspect::LineEditDisplay);
    setLabelText(Tr::tr("Document Url:"));
}

AppManagerControllerAspect::AppManagerControllerAspect(Utils::AspectContainer *container)
    : FilePathAspect(container)
{
    setSettingsKey("ApplicationManagerPlugin.AppControllerPath");
    setLabelText(Tr::tr("Controller:"));
    setPlaceHolderText(Tr::tr("-"));
}

AppManagerStringAspect::AppManagerStringAspect(AspectContainer *container)
    : StringAspect(container)
{
}

QString AppManagerStringAspect::valueOrDefault(const QString &defaultValue) const
{
    return value().isEmpty() ? defaultValue : value();
}

// FilePath

AppManagerFilePathAspect::AppManagerFilePathAspect(AspectContainer *container)
    : FilePathAspect(container)
{
}

void AppManagerFilePathAspect::setButtonsVisible(bool visible)
{
    if (m_buttonsVisibile != visible) {
        m_buttonsVisibile = visible;
        updateWidgets();
    }
}

void AppManagerFilePathAspect::setPlaceHolderPath(const QString &value)
{
    if (m_placeHolderPath != value) {
        m_placeHolderPath = value;
        updateWidgets();
    }
}

void AppManagerFilePathAspect::setPromptDialogFilter(const QString &value)
{
    if (m_promptDialogFilter != value) {
        m_promptDialogFilter = value;
        updateWidgets();
    }
}

void AppManagerFilePathAspect::addToLayout(Layouting::LayoutItem &parent)
{
    FilePathAspect::addToLayout(parent);
    updateWidgets();
}

FilePath AppManagerFilePathAspect::valueOrDefault(const FilePath &defaultValue) const
{
    return operator()().isEmpty() ? defaultValue : operator()();
}

FilePath AppManagerFilePathAspect::valueOrDefault(const QString &defaultValue) const
{
    return operator()().isEmpty() ? FilePath::fromUserInput(defaultValue) : operator()();
}

static bool callValidationFunction(const FancyLineEdit::ValidationFunction &f, FancyLineEdit *lineEdit, QString *errorMessage)
{
    if (f.index() == 0) {
        QString oldText = lineEdit->text();
        auto future = std::get<0>(f)(oldText);
        future.waitForFinished();
        auto res = future.result();
        if (res) {
            if (oldText != *res && lineEdit->text() == *res)
                lineEdit->setText(*res);
            return true;
        }

        if (errorMessage)
            *errorMessage = res.error();

        return false;
    }
    return std::get<1>(f)(lineEdit, errorMessage);
}

bool AppManagerFilePathAspect::validatePathWithPlaceHolder(FancyLineEdit *lineEdit, QString *errorMessage) const
{
    if (!lineEdit)
        return false;
    if (auto pathChooser = qobject_cast<PathChooser*>(lineEdit->parent())) {
        if (lineEdit->text().isEmpty() && !lineEdit->placeholderText().isEmpty()) {
            FancyLineEdit temporaryLineEdit;
            temporaryLineEdit.setText(lineEdit->placeholderText());
            PathChooser temporaryPathChooser;
            temporaryPathChooser.setExpectedKind(pathChooser->expectedKind());
            return callValidationFunction(temporaryPathChooser.defaultValidationFunction(), &temporaryLineEdit, errorMessage);
        }
        return callValidationFunction(pathChooser->defaultValidationFunction(), lineEdit, errorMessage);
    }
    return callValidationFunction(lineEdit->defaultValidationFunction(), lineEdit, errorMessage);
}

void AppManagerFilePathAspect::updateWidgets()
{
    const auto pathChooser = this->pathChooser();
    if (!pathChooser)
        return;
    for (auto button : pathChooser->findChildren<QPushButton*>())
        button->setVisible(m_buttonsVisibile);
    for (auto fancyLineEdit : pathChooser->findChildren<FancyLineEdit*>())
        fancyLineEdit->setPlaceholderText(m_placeHolderPath);
    QFileInfo initialBrowsePath(m_placeHolderPath);
    while (!initialBrowsePath.path().isEmpty() && !initialBrowsePath.isDir())
        initialBrowsePath.setFile(initialBrowsePath.path());
    if (initialBrowsePath.isDir())
        pathChooser->setInitialBrowsePathBackup(FilePath::fromString(initialBrowsePath.absoluteFilePath()));
    pathChooser->setPromptDialogFilter(m_promptDialogFilter);
    pathChooser->setValidationFunction(
        [&](FancyLineEdit *edit, QString *error) { return validatePathWithPlaceHolder(edit, error); });
}

} // namespace Internal
} // namespace AppManager
