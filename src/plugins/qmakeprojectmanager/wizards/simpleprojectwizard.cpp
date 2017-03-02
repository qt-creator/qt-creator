/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "simpleprojectwizard.h"

#include <qmakeprojectmanager/qmakeprojectmanagerconstants.h>

#include <coreplugin/basefilewizard.h>
#include <coreplugin/icore.h>

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/customwizard/customwizard.h>
#include <projectexplorer/selectablefilesmodel.h>

#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/filewizardpage.h>
#include <utils/mimetypes/mimedatabase.h>
#include <utils/wizard.h>

#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QPainter>
#include <QPixmap>
#include <QStyle>
#include <QVBoxLayout>
#include <QWizardPage>

using namespace Core;
using namespace ProjectExplorer;
using namespace Utils;

namespace QmakeProjectManager {
namespace Internal {

class SimpleProjectWizardDialog;

class FilesSelectionWizardPage : public QWizardPage
{
    Q_OBJECT

public:
    FilesSelectionWizardPage(SimpleProjectWizardDialog *simpleProjectWizard);
    bool isComplete() const override { return m_filesWidget->hasFilesSelected(); }
    void initializePage() override;
    void cleanupPage() override { m_filesWidget->cancelParsing(); }
    FileNameList selectedFiles() const { return m_filesWidget->selectedFiles(); }
    FileNameList selectedPaths() const { return m_filesWidget->selectedPaths(); }

private:
    SimpleProjectWizardDialog *m_simpleProjectWizardDialog;
    SelectableFilesWidget *m_filesWidget;
};

FilesSelectionWizardPage::FilesSelectionWizardPage(SimpleProjectWizardDialog *simpleProjectWizard)
    : m_simpleProjectWizardDialog(simpleProjectWizard),
      m_filesWidget(new SelectableFilesWidget(this))
{
    auto layout = new QVBoxLayout(this);

    layout->addWidget(m_filesWidget);
    m_filesWidget->setBaseDirEditable(false);
    connect(m_filesWidget, &SelectableFilesWidget::selectedFilesChanged,
            this, &FilesSelectionWizardPage::completeChanged);

    setProperty(Utils::SHORT_TITLE_PROPERTY, tr("Files"));
}

class SimpleProjectWizardDialog : public BaseFileWizard
{
    Q_OBJECT

public:
    SimpleProjectWizardDialog(const BaseFileWizardFactory *factory, QWidget *parent)
        : BaseFileWizard(factory, QVariantMap(), parent)
    {
        setWindowTitle(tr("Import Existing Project"));

        m_firstPage = new FileWizardPage;
        m_firstPage->setTitle(tr("Project Name and Location"));
        m_firstPage->setFileNameLabel(tr("Project name:"));
        m_firstPage->setPathLabel(tr("Location:"));
        addPage(m_firstPage);

        m_secondPage = new FilesSelectionWizardPage(this);
        m_secondPage->setTitle(tr("File Selection"));
        addPage(m_secondPage);
    }

    QString path() const { return m_firstPage->path(); }
    void setPath(const QString &path) { m_firstPage->setPath(path); }
    FileNameList selectedFiles() const { return m_secondPage->selectedFiles(); }
    FileNameList selectedPaths() const { return m_secondPage->selectedPaths(); }
    QString projectName() const { return m_firstPage->fileName(); }

    FileWizardPage *m_firstPage;
    FilesSelectionWizardPage *m_secondPage;
};

void FilesSelectionWizardPage::initializePage()
{
    m_filesWidget->resetModel(FileName::fromString(m_simpleProjectWizardDialog->path()),
                              FileNameList());
}

SimpleProjectWizard::SimpleProjectWizard()
{
    setSupportedProjectTypes({Constants::PROJECT_ID});
    // TODO do something about the ugliness of standard icons in sizes different than 16, 32, 64, 128
    {
        QPixmap icon(22, 22);
        icon.fill(Qt::transparent);
        QPainter p(&icon);
        p.drawPixmap(3, 3, 16, 16, qApp->style()->standardIcon(QStyle::SP_DirIcon).pixmap(16));
        setIcon(icon);
    }
    setDisplayName(tr("Import as qmake Project (Limited Functionality)"));
    setId("Z.DummyProFile");
    setDescription(tr("Imports existing projects that do not use qmake, CMake or Autotools.<p>"
                      "This creates a qmake .pro file that allows you to use Qt Creator as a code editor "
                      "and as a launcher for debugging and analyzing tools. "
                      "If you want to build the project, you might need to edit the generated .pro file."));
    setCategory(ProjectExplorer::Constants::IMPORT_WIZARD_CATEGORY);
    setDisplayCategory(ProjectExplorer::Constants::IMPORT_WIZARD_CATEGORY_DISPLAY);
    setFlags(IWizardFactory::PlatformIndependent);
}

BaseFileWizard *SimpleProjectWizard::create(QWidget *parent,
                                            const WizardDialogParameters &parameters) const
{
    auto wizard = new SimpleProjectWizardDialog(this, parent);
    wizard->setPath(parameters.defaultPath());

    for (QWizardPage *p : wizard->extensionPages())
        wizard->addPage(p);

    return wizard;
}

GeneratedFiles SimpleProjectWizard::generateFiles(const QWizard *w,
                                                  QString *errorMessage) const
{
    Q_UNUSED(errorMessage)

    auto wizard = qobject_cast<const SimpleProjectWizardDialog *>(w);
    const QString projectPath = wizard->path();
    const QDir dir(projectPath);
    const QString projectName = wizard->projectName();
    const QString proFileName = QFileInfo(dir, projectName + ".pro").absoluteFilePath();
    const QStringList paths = Utils::transform(wizard->selectedPaths(), &FileName::toString);

    MimeType headerType = Utils::mimeTypeForName("text/x-chdr");

    QStringList nameFilters = headerType.globPatterns();

    QString proIncludes = "INCLUDEPATH = \\\n";
    for (const QString &path : paths) {
        QFileInfo fileInfo(path);
        QDir thisDir(fileInfo.absoluteFilePath());
        if (!thisDir.entryList(nameFilters, QDir::Files).isEmpty()) {
            QString relative = dir.relativeFilePath(path);
            if (!relative.isEmpty())
                proIncludes.append("    $$PWD/" + relative + " \\\n");
        }
    }

    QString proSources = "SOURCES = \\\n";
    QString proHeaders = "HEADERS = \\\n";

    for (const FileName &fileName : wizard->selectedFiles()) {
        QString source = dir.relativeFilePath(fileName.toString());
        MimeType mimeType = Utils::mimeTypeForFile(fileName.toFileInfo());
        if (mimeType.matchesName("text/x-chdr") || mimeType.matchesName("text/x-c++hdr"))
            proHeaders += "   $$PWD/" + source + " \\\n";
        else
            proSources += "   $$PWD/" + source + " \\\n";
    }

    proHeaders.chop(3);
    proSources.chop(3);
    proIncludes.chop(3);

    GeneratedFile generatedProFile(proFileName);
    generatedProFile.setAttributes(Core::GeneratedFile::OpenProjectAttribute);
    generatedProFile.setContents(
        "# Created by and for Qt Creator. This file was created for editing the project sources only.\n"
        "# You may attempt to use it for building too, by modifying this file here.\n\n"
        "#TARGET = " + projectName + "\n\n"
        + proHeaders + "\n\n"
        + proSources + "\n\n"
        + proIncludes + "\n\n"
        "#DEFINES = \n\n"
    );

    return GeneratedFiles{generatedProFile};
}

bool SimpleProjectWizard::postGenerateFiles(const QWizard *w, const GeneratedFiles &l,
                                             QString *errorMessage) const
{
    Q_UNUSED(w);
    return CustomProjectWizard::postGenerateOpen(l, errorMessage);
}

} // namespace Internal
} // namespace QmakeProjectManager

#include "simpleprojectwizard.moc"
