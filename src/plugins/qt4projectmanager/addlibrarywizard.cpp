#include "addlibrarywizard.h"
#include "ui_librarydetailswidget.h"
#include "librarydetailscontroller.h"

#include <QtGui/QVBoxLayout>
#include <QtGui/QRadioButton>
#include <QtGui/QLabel>
#include <QtCore/QFileInfo>

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
    setAutomaticProgressCreationEnabled(false);
    m_libraryTypePage = new LibraryTypePage(this);
    m_detailsPage = new DetailsPage(this);
    m_summaryPage = new SummaryPage(this);
    setPage(LibraryTypePageId, m_libraryTypePage);
    setPage(DetailsPageId, m_detailsPage);
    setPage(SummaryPageId, m_summaryPage);

    Utils::WizardProgress *progress = wizardProgress();

    Utils::WizardProgressItem *kindItem = progress->addItem(tr("Type"));

    Utils::WizardProgressItem *detailsItem = progress->addItem(tr("Details"));
    Utils::WizardProgressItem *summaryItem = progress->addItem(tr("Summary"));

    kindItem->addPage(LibraryTypePageId);
    detailsItem->addPage(DetailsPageId);
    summaryItem->addPage(SummaryPageId);

    kindItem->setNextItems(QList<Utils::WizardProgressItem *>() << detailsItem);
    detailsItem->setNextItems(QList<Utils::WizardProgressItem *>() << summaryItem);

    setStartId(LibraryTypePageId);
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

    m_systemRadio = new QRadioButton(tr("System library"), this);
    m_systemRadio->setChecked(true);
    layout->addWidget(m_systemRadio);

    QLabel *systemLabel = new QLabel(tr("Links to a system library."
                                    "\nNeither the path to the "
                                    "library nor the path to its "
                                    "includes is added to the .pro file."));

    systemLabel->setWordWrap(true);
    systemLabel->setAttribute(Qt::WA_MacSmallSize, true);
    layout->addWidget(systemLabel);

    m_externalRadio = new QRadioButton(tr("External library"), this);
    layout->addWidget(m_externalRadio);

    QLabel *externalLabel = new QLabel(tr("Links to a library "
                                    "that is not located in your "
                                    "build tree.\nAdds the library "
                                    "and include paths to the .pro file."));

    externalLabel->setWordWrap(true);
    externalLabel->setAttribute(Qt::WA_MacSmallSize, true);
    layout->addWidget(externalLabel);

    m_internalRadio = new QRadioButton(tr("Internal library"), this);
    layout->addWidget(m_internalRadio);

    QLabel *internalLabel = new QLabel(tr("Links to a library "
                                    "that is located in your build "
                                    "tree.\nAdds the library and "
                                    "include paths to the .pro file."));

    internalLabel->setWordWrap(true);
    internalLabel->setAttribute(Qt::WA_MacSmallSize, true);
    layout->addWidget(internalLabel);
}

AddLibraryWizard::LibraryKind LibraryTypePage::libraryKind() const
{
    if (m_internalRadio->isChecked())
        return AddLibraryWizard::InternalLibrary;
    if (m_systemRadio->isChecked())
        return AddLibraryWizard::SystemLibrary;
    return AddLibraryWizard::ExternalLibrary;
}

int LibraryTypePage::nextId() const
{
    return AddLibraryWizard::DetailsPageId;
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

int DetailsPage::nextId() const
{
    return AddLibraryWizard::SummaryPageId;
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
    case AddLibraryWizard::SystemLibrary:
        title = tr("System Library");
        subTitle = tr("Specify the library to link to");
        m_libraryDetailsController = new SystemLibraryDetailsController(
                m_libraryDetailsWidget, m_libraryWizard->proFile(), this);
        break;
    case AddLibraryWizard::ExternalLibrary:
        title = tr("External Library");
        subTitle = tr("Specify the library to link to and the includes path");
        m_libraryDetailsController = new ExternalLibraryDetailsController(
                m_libraryDetailsWidget, m_libraryWizard->proFile(), this);
        break;
    case AddLibraryWizard::InternalLibrary:
        title = tr("Internal Library");
        subTitle = tr("Choose the project file of the library to link to");
        m_libraryDetailsController = new InternalLibraryDetailsController(
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
        str << text;
        str << "</code>";
    }

    m_snippetLabel->setText(richSnippet);
}

QString SummaryPage::snippet() const
{
    return m_snippet;
}
