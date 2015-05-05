//
// Copyright (C) 2013 Mateusz Łoskot <mateusz@loskot.net>
// Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
//
// This file is part of Qt Creator Boost.Build plugin project.
//
// This is free software; you can redistribute and/or modify it under
// the terms of the GNU Lesser General Public License, Version 2.1
// as published by the Free Software Foundation.
// See accompanying file LICENSE.txt or copy at
// http://www.gnu.org/licenses/lgpl-2.1-standalone.html.
//
#ifndef BBOPENPROJECTWIZARD_HPP
#define BBOPENPROJECTWIZARD_HPP

// Qt Creator
#include <coreplugin/basefilewizard.h>
#include <coreplugin/basefilewizardfactory.h>
#include <coreplugin/generatedfile.h>
#include <utils/wizard.h>
// Qt
#include <QString>
#include <QStringList>
QT_BEGIN_NAMESPACE
class QVBoxLayout;
class QLabel;
class QTreeView;
class QLineEdit;
QT_END_NAMESPACE

namespace Utils {
class PathChooser;
}

namespace BoostBuildProjectManager {
namespace Internal {

class Project;
class PathsSelectionWizardPage;
class FilesSelectionWizardPage;

//////////////////////////////////////////////////////////////////////////////////////////
// NOTE: The Boost.Build wizard is based on Core::BaseFileWizard which seems to be
// dedicated to build "New Project" wizards. So, the plugin uses the base class in
// unconventional, matching its features to Boost.Build wizard needs, like:
// - no parent QWidget is used
// - platform name is set from default Kit display name, usually it's "Desktop"
// - extra values QVariantMap may carry custom data
// CAUTION: This wizard may stop building or start failing in run-time,
// if Qt Creator changes the base class significantly.
class OpenProjectWizard : public Core::BaseFileWizardFactory
{
    Q_OBJECT

public:
    OpenProjectWizard(Project const* const project);

    bool run(QString const& platform, QVariantMap const& extraValues);

    QVariantMap outputValues() const { return outputValues_; }

protected:

    Core::BaseFileWizard*
    create(QWidget* parent, Core::WizardDialogParameters const& parameters) const;

    Core::GeneratedFiles
    generateFiles(QWizard const* baseWizard, QString* errorMessage) const;

    bool
    postGenerateFiles(QWizard const* wizard
        , Core::GeneratedFiles const& files, QString* errorMessage);

private:
    Project const* const project_;
    QVariantMap outputValues_;
    bool projectOpened_;
};

//////////////////////////////////////////////////////////////////////////////////////////
class OpenProjectWizardDialog : public Core::BaseFileWizard
{
    Q_OBJECT

public:
    OpenProjectWizardDialog(QWidget* parent, QString const& projectFile
        , QVariantMap const& extraValues, QVariantMap& outputValues);

    QString path() const;
    QString projectFile() const;
    QString projectName() const;
    QString defaultProjectName() const;

    QStringList selectedFiles() const;
    QStringList selectedPaths() const;

public slots:
    void setProjectName(QString const& name);

private:
    QVariantMap& outputValues_;
    QVariantMap extraValues_;
    QString projectFile_;
    PathsSelectionWizardPage* pathsPage_;
    FilesSelectionWizardPage* filesPage_;
};

//////////////////////////////////////////////////////////////////////////////////////////
class PathsSelectionWizardPage : public QWizardPage
{
    Q_OBJECT

public:
    explicit PathsSelectionWizardPage(OpenProjectWizardDialog* wizard);

    QString projectName() const;
    void setProjectName(QString const& name);

private:
    OpenProjectWizardDialog* wizard_;
    QLineEdit* nameLineEdit_;
};

} // namespace Internal
} // namespace BoostBuildProjectManager

#endif // BBOPENPROJECTWIZARD_HPP
