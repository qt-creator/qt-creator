/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "addlibrarywizard.h"
#include "ui_librarydetailswidget.h"
#include "librarydetailscontroller.h"

#include <utils/hostosinfo.h>

#include <QVBoxLayout>
#include <QRadioButton>
#include <QLabel>

#include <QFileInfo>
#include <QDebug>

using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Internal;


const char *qt_file_dialog_filter_reg_exp =
"^(.*)\\(([a-zA-Z0-9_.*? +;#\\-\\[\\]@\\{\\}/!<>\\$%&=^~:\\|]*)\\)$";

// taken from qfiledialog.cpp
QStringList qt_clean_filter_list(const QString &filter)
{
    QRegExp regexp(QString::fromLatin1(qt_file_dialog_filter_reg_exp));
    QString f = filter;
    int i = regexp.indexIn(f);
    if (i >= 0)
        f = regexp.cap(2);
    return f.split(QLatin1Char(' '), QString::SkipEmptyParts);
}

LibraryPathChooser::LibraryPathChooser(QWidget *parent)
    : Utils::PathChooser(parent)
{
}

bool LibraryPathChooser::validatePath(const QString &path, QString *errorMessage)
{
    bool result = PathChooser::validatePath(path, errorMessage);
    if (!result)
        return false;

    QFileInfo fi(path);
    if (!fi.exists())
        return false;

    const QString fileName = fi.fileName();

    QStringList filters = qt_clean_filter_list(promptDialogFilter());
    for (int i = 0; i < filters.count(); i++) {
        QRegExp regExp(filters.at(i));
        regExp.setPatternSyntax(QRegExp::Wildcard);
        if (regExp.exactMatch(fileName))
            return true;
        }
    return false;
}

AddLibraryWizard::AddLibraryWizard(const QString &fileName, QWidget *parent) :
    Utils::Wizard(parent), m_proFile(fileName)
{
    setWindowTitle(tr("Add Library"));
    m_libraryTypePage = new LibraryTypePage(this);
    m_detailsPage = new DetailsPage(this);
    m_summaryPage = new SummaryPage(this);
    const int libraryTypePageId = addPage(m_libraryTypePage);
    const int detailsPageId = addPage(m_detailsPage);
    const int summaryPageId = addPage(m_summaryPage);

    Utils::WizardProgress *progress = wizardProgress();

    progress->item(libraryTypePageId)->setTitle(tr("Type"));
    progress->item(detailsPageId)->setTitle(tr("Details"));
    progress->item(summaryPageId)->setTitle(tr("Summary"));
}

AddLibraryWizard::~AddLibraryWizard()
{
}

QString AddLibraryWizard::proFile() const
{
    return m_proFile;
}

AddLibraryWizard::LibraryKind AddLibraryWizard::libraryKind() const
{
    return m_libraryTypePage->libraryKind();
}

QString AddLibraryWizard::snippet() const
{
    return m_detailsPage->snippet();
}

/////////////

LibraryTypePage::LibraryTypePage(AddLibraryWizard *parent)
    : QWizardPage(parent)
{
    setTitle(tr("Library Type"));
    setSubTitle(tr("Choose the type of the library to link to"));

    QVBoxLayout *layout = new QVBoxLayout(this);

    m_internalRadio = new QRadioButton(tr("Internal library"), this);
    layout->addWidget(m_internalRadio);

    QLabel *internalLabel = new QLabel(tr("Links to a library "
                                    "that is located in your build "
                                    "tree.\nAdds the library and "
                                    "include paths to the .pro file."));

    internalLabel->setWordWrap(true);
    internalLabel->setAttribute(Qt::WA_MacSmallSize, true);
    layout->addWidget(internalLabel);

    m_externalRadio = new QRadioButton(tr("External library"), this);
    layout->addWidget(m_externalRadio);

    QLabel *externalLabel = new QLabel(tr("Links to a library "
                                    "that is not located in your "
                                    "build tree.\nAdds the library "
                                    "and include paths to the .pro file."));

    externalLabel->setWordWrap(true);
    externalLabel->setAttribute(Qt::WA_MacSmallSize, true);
    layout->addWidget(externalLabel);

    m_systemRadio = new QRadioButton(tr("System library"), this);
    layout->addWidget(m_systemRadio);

    QLabel *systemLabel = new QLabel(tr("Links to a system library."
                                    "\nNeither the path to the "
                                    "library nor the path to its "
                                    "includes is added to the .pro file."));

    systemLabel->setWordWrap(true);
    systemLabel->setAttribute(Qt::WA_MacSmallSize, true);
    layout->addWidget(systemLabel);

    m_packageRadio = new QRadioButton(tr("System package"), this);
    layout->addWidget(m_packageRadio);

    QLabel *packageLabel = new QLabel(tr("Links to a system library using pkg-config."));

    packageLabel->setWordWrap(true);
    packageLabel->setAttribute(Qt::WA_MacSmallSize, true);
    layout->addWidget(packageLabel);

    if (Utils::HostOsInfo::isWindowsHost()) {
        m_packageRadio->setVisible(false);
        packageLabel->setVisible(false);
    }

    // select the default
    m_internalRadio->setChecked(true);
}

AddLibraryWizard::LibraryKind LibraryTypePage::libraryKind() const
{
    if (m_internalRadio->isChecked())
        return AddLibraryWizard::InternalLibrary;
    if (m_externalRadio->isChecked())
        return AddLibraryWizard::ExternalLibrary;
    if (m_systemRadio->isChecked())
        return AddLibraryWizard::SystemLibrary;
    return AddLibraryWizard::PackageLibrary;
}

/////////////

DetailsPage::DetailsPage(AddLibraryWizard *parent)
    : QWizardPage(parent), m_libraryWizard(parent), m_libraryDetailsController(0)
{
    m_libraryDetailsWidget = new Ui::LibraryDetailsWidget();
    m_libraryDetailsWidget->setupUi(this);
}

bool DetailsPage::isComplete() const
{
    if (m_libraryDetailsController)
        return m_libraryDetailsController->isComplete();
    return false;
}

QString DetailsPage::snippet() const
{
    if (m_libraryDetailsController)
        return m_libraryDetailsController->snippet();
    return QString();
}

void DetailsPage::initializePage()
{
    if (m_libraryDetailsController) {
        delete m_libraryDetailsController;
        m_libraryDetailsController = 0;
    }
    QString title;
    QString subTitle;
    switch (m_libraryWizard->libraryKind()) {
    case AddLibraryWizard::InternalLibrary:
        title = tr("Internal Library");
        subTitle = tr("Choose the project file of the library to link to");
        m_libraryDetailsController = new InternalLibraryDetailsController(
                m_libraryDetailsWidget, m_libraryWizard->proFile(), this);
        break;
    case AddLibraryWizard::ExternalLibrary:
        title = tr("External Library");
        subTitle = tr("Specify the library to link to and the includes path");
        m_libraryDetailsController = new ExternalLibraryDetailsController(
                m_libraryDetailsWidget, m_libraryWizard->proFile(), this);
        break;
    case AddLibraryWizard::SystemLibrary:
        title = tr("System Library");
        subTitle = tr("Specify the library to link to");
        m_libraryDetailsController = new SystemLibraryDetailsController(
                m_libraryDetailsWidget, m_libraryWizard->proFile(), this);
        break;
    case AddLibraryWizard::PackageLibrary:
        title = tr("System Package");
        subTitle = tr("Specify the package to link to");
        m_libraryDetailsController = new PackageLibraryDetailsController(
                m_libraryDetailsWidget, m_libraryWizard->proFile(), this);
        break;
    default:
        break;
    }
    setTitle(title);
    setSubTitle(subTitle);
    if (m_libraryDetailsController) {
        connect(m_libraryDetailsController, SIGNAL(completeChanged()),
                this, SIGNAL(completeChanged()));
    }
}

/////////////

SummaryPage::SummaryPage(AddLibraryWizard *parent)
    : QWizardPage(parent), m_libraryWizard(parent)
{
    setTitle(tr("Summary"));
    setFinalPage(true);

    QVBoxLayout *layout = new QVBoxLayout(this);
    m_summaryLabel = new QLabel(this);
    m_snippetLabel = new QLabel(this);
    layout->addWidget(m_summaryLabel);
    layout->addWidget(m_snippetLabel);
    m_summaryLabel->setTextFormat(Qt::RichText);
    m_snippetLabel->setTextFormat(Qt::RichText);
    m_snippetLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
}

void SummaryPage::initializePage()
{
    m_snippet = m_libraryWizard->snippet();
    QFileInfo fi(m_libraryWizard->proFile());
    m_summaryLabel->setText(
            tr("The following snippet will be added to the<br><b>%1</b> file:")
            .arg(fi.fileName()));
    QString richSnippet;
    {
        QTextStream str(&richSnippet);
        str << "<code>";
        QString text = m_snippet;
        text.replace(QLatin1Char('\n'), QLatin1String("<br>"));
        text.replace(QLatin1Char(' '), QLatin1String("&nbsp;"));
        str << text;
        str << "</code>";
    }

    m_snippetLabel->setText(richSnippet);
}

QString SummaryPage::snippet() const
{
    return m_snippet;
}
