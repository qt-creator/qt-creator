// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "addlibrarywizard.h"

#include "librarydetailscontroller.h"

#include <utils/filepath.h>
#include <utils/hostosinfo.h>
#include <utils/layoutbuilder.h>
#include <utils/pathchooser.h>
#include <utils/stringutils.h>

#include <QCheckBox>
#include <QComboBox>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QRadioButton>
#include <QRadioButton>
#include <QScrollArea>
#include <QTextStream>
#include <QVBoxLayout>

namespace QmakeProjectManager::Internal {

const char qt_file_dialog_filter_reg_exp[] =
"^(.*)\\(([a-zA-Z0-9_.*? +;#\\-\\[\\]@\\{\\}/!<>\\$%&=^~:\\|]*)\\)$";

static QStringList qt_clean_filter_list(const QString &filter)
{
    const QRegularExpression regexp(qt_file_dialog_filter_reg_exp);
    const QRegularExpressionMatch match = regexp.match(filter);
    QString f = filter;
    if (match.hasMatch())
        f = match.captured(2);
    return f.split(QLatin1Char(' '), Qt::SkipEmptyParts);
}

static bool validateLibraryPath(const Utils::FilePath &filePath,
                                const Utils::PathChooser *pathChooser,
                                QString *errorMessage)
{
    Q_UNUSED(errorMessage)
    if (!filePath.exists())
        return false;

    const QString fileName = filePath.fileName();

    QRegularExpression::PatternOption option =
        Utils::HostOsInfo::fileNameCaseSensitivity() == Qt::CaseInsensitive
            ? QRegularExpression::CaseInsensitiveOption
            : QRegularExpression::NoPatternOption;

    const QStringList filters = qt_clean_filter_list(pathChooser->promptDialogFilter());
    for (const QString &filter : filters) {
        QString pattern = QRegularExpression::wildcardToRegularExpression(filter);
        QRegularExpression regExp(pattern, option);
        if (regExp.match(fileName).hasMatch())
            return true;
        }
    return false;
}

AddLibraryWizard::AddLibraryWizard(const Utils::FilePath &proFile, QWidget *parent) :
    Utils::Wizard(parent), m_proFile(proFile)
{
    setWindowTitle(tr("Add Library"));
    m_libraryTypePage = new LibraryTypePage(this);
    addPage(m_libraryTypePage);
    m_detailsPage = new DetailsPage(this);
    addPage(m_detailsPage);
    m_summaryPage = new SummaryPage(this);
    addPage(m_summaryPage);
}

AddLibraryWizard::~AddLibraryWizard() = default;

Utils::FilePath AddLibraryWizard::proFile() const
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

    auto *layout = new QVBoxLayout(this);

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

    setProperty(Utils::SHORT_TITLE_PROPERTY, tr("Type"));
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
    : QWizardPage(parent), m_libraryWizard(parent)
{
    m_libraryDetailsWidget = new LibraryDetailsWidget(this);
    resize(456, 438);

    Utils::PathChooser * const libPathChooser = m_libraryDetailsWidget->libraryPathChooser;
    libPathChooser->setHistoryCompleter("Qmake.LibDir.History");

    const auto pathValidator = [libPathChooser](Utils::FancyLineEdit *edit, QString *errorMessage) {
        return libPathChooser->defaultValidationFunction()(edit, errorMessage)
                && validateLibraryPath(libPathChooser->filePath(),
                                       libPathChooser, errorMessage);
    };
    libPathChooser->setValidationFunction(pathValidator);
    setProperty(Utils::SHORT_TITLE_PROPERTY, tr("Details"));
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
        m_libraryDetailsController = nullptr;
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
        connect(m_libraryDetailsController, &LibraryDetailsController::completeChanged,
                this, &QWizardPage::completeChanged);
    }
}

/////////////

SummaryPage::SummaryPage(AddLibraryWizard *parent)
    : QWizardPage(parent), m_libraryWizard(parent)
{
    setTitle(tr("Summary"));
    setFinalPage(true);

    auto *layout = new QVBoxLayout(this);
    const auto scrollArea = new QScrollArea;
    const auto snippetWidget = new QWidget;
    const auto snippetLayout = new QVBoxLayout(snippetWidget);
    m_summaryLabel = new QLabel(this);
    m_snippetLabel = new QLabel(this);
    m_snippetLabel->setWordWrap(true);
    layout->addWidget(m_summaryLabel);
    snippetLayout->addWidget(m_snippetLabel);
    snippetLayout->addStretch(1);
    scrollArea->setWidget(snippetWidget);
    scrollArea->setWidgetResizable(true);
    layout->addWidget(scrollArea);
    m_summaryLabel->setTextFormat(Qt::RichText);
    m_snippetLabel->setTextFormat(Qt::RichText);
    m_snippetLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);

    setProperty(Utils::SHORT_TITLE_PROPERTY, tr("Summary"));
}

void SummaryPage::initializePage()
{
    m_snippet = m_libraryWizard->snippet();
    m_summaryLabel->setText(
            tr("The following snippet will be added to the<br><b>%1</b> file:")
            .arg(m_libraryWizard->proFile().fileName()));
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

LibraryDetailsWidget::LibraryDetailsWidget(QWidget *parent)
{
    includePathChooser = new Utils::PathChooser(parent);

    packageLineEdit = new QLineEdit(parent);

    libraryPathChooser = new Utils::PathChooser(parent);

    libraryComboBox = new QComboBox(parent);

    libraryTypeComboBox = new QComboBox(parent);


    platformGroupBox = new QGroupBox(tr("Platform:"));
    platformGroupBox->setFlat(true);

    linkageGroupBox = new QGroupBox(tr("Linkage:"));
    linkageGroupBox->setFlat(true);

    macGroupBox = new QGroupBox(tr("Mac:"));
    macGroupBox->setFlat(true);

    winGroupBox = new QGroupBox(tr("Windows:"));
    winGroupBox->setFlat(true);


    linCheckBox = new QCheckBox(tr("Linux"));
    linCheckBox->setChecked(true);

    macCheckBox = new QCheckBox(tr("Mac"));
    macCheckBox->setChecked(true);

    winCheckBox = new QCheckBox(tr("Windows"));
    winCheckBox->setChecked(true);

    dynamicRadio = new QRadioButton(tr("Dynamic"), linkageGroupBox);
    staticRadio = new QRadioButton(tr("Static"), linkageGroupBox);

    libraryRadio = new QRadioButton(tr("Library"), macGroupBox);
    frameworkRadio = new QRadioButton(tr("Framework"), macGroupBox);

    useSubfoldersCheckBox = new QCheckBox(tr("Library inside \"debug\" or \"release\" subfolder"),
        winGroupBox);
    useSubfoldersCheckBox->setChecked(true);

    addSuffixCheckBox = new QCheckBox(tr("Add \"d\" suffix for debug version"), winGroupBox);
    removeSuffixCheckBox = new QCheckBox(tr("Remove \"d\" suffix for release version"), winGroupBox);

    using namespace Utils::Layouting;

    Column { linCheckBox, macCheckBox, winCheckBox, st }.attachTo(platformGroupBox);

    Row { dynamicRadio, staticRadio }.attachTo(linkageGroupBox);

    Row { libraryRadio, frameworkRadio }.attachTo(macGroupBox);

    Column { useSubfoldersCheckBox, addSuffixCheckBox, removeSuffixCheckBox }.attachTo(winGroupBox);

    libraryLabel = new QLabel(tr("Library:"));
    libraryFileLabel = new QLabel(tr("Library file:"));
    libraryTypeLabel = new QLabel(tr("Library type:"));
    packageLabel = new QLabel(tr("Package:"));
    includeLabel = new QLabel(tr("Include path:"));

    Column {
        Form {
            libraryLabel, libraryComboBox, br,
            libraryTypeLabel, libraryTypeComboBox, br,
            libraryFileLabel, libraryPathChooser, br,
            packageLabel, packageLineEdit, br,
            includeLabel, includePathChooser
        },
        Row {
            platformGroupBox,
            Column {
                linkageGroupBox,
                macGroupBox,
                winGroupBox,
            }
        },
        st
    }.attachTo(parent);
}

} // QmakeProjectManager::Internal
