// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "squishwizardpages.h"

#include "squishfilehandler.h"
#include "squishsettings.h"
#include "squishtools.h"
#include "squishtr.h"

#include <coreplugin/icore.h>

#include <utils/infolabel.h>
#include <utils/qtcassert.h>

#include <QApplication>
#include <QButtonGroup>
#include <QComboBox>
#include <QGroupBox>
#include <QLineEdit>
#include <QRadioButton>
#include <QTimer>
#include <QVBoxLayout>

namespace Squish {
namespace Internal {

/************************************ ToolkitsPage ***********************************************/

SquishToolkitsPageFactory::SquishToolkitsPageFactory()
{
    setTypeIdsSuffix("SquishToolkits");
}

Utils::WizardPage *SquishToolkitsPageFactory::create(ProjectExplorer::JsonWizard *,
                                                     Utils::Id typeId, const QVariant &)
{
    QTC_ASSERT(canCreate(typeId), return nullptr);
    return new SquishToolkitsPage;
}

bool SquishToolkitsPageFactory::validateData(Utils::Id typeId, const QVariant &, QString *)
{
    QTC_ASSERT(canCreate(typeId), return false);
    return true;
}

SquishToolkitsPage::SquishToolkitsPage()
{
    setTitle(Tr::tr("Create New Squish Test Suite"));

    auto layout = new QVBoxLayout(this);
    auto groupBox = new QGroupBox(Tr::tr("Available GUI toolkits:"), this);
    auto buttonLayout = new QVBoxLayout(groupBox);

    m_buttonGroup = new QButtonGroup(this);
    m_buttonGroup->setExclusive(true);
    const QStringList toolkits = { "Android", "iOS", "Java", "Mac", "Qt", "Tk", "VNC", "Windows",
                                   "Web", "XView"};
    for (const QString &toolkit : toolkits) {
        auto button = new QRadioButton(toolkit, this);
        button->setEnabled(false);
        m_buttonGroup->addButton(button);
        buttonLayout->addWidget(button);
    }
    groupBox->setLayout(buttonLayout);
    layout->addWidget(groupBox);

    m_errorLabel = new Utils::InfoLabel(Tr::tr("Invalid Squish settings. Configure Squish "
                                               "installation path inside "
                                               "Preferences... > Squish > General to use "
                                               "this wizard."),
                                        Utils::InfoLabel::Error, this);
    m_errorLabel->setVisible(false);
    layout->addWidget(m_errorLabel);

    auto hiddenLineEdit = new QLineEdit(this);
    hiddenLineEdit->setVisible(false);
    layout->addWidget(hiddenLineEdit);
    registerFieldWithName("ChosenToolkit", hiddenLineEdit);

    m_hiddenLineEdit = new QLineEdit(this);
    m_hiddenLineEdit->setVisible(false);
    layout->addWidget(m_hiddenLineEdit);
    registerField("RegisteredAUTs", m_hiddenLineEdit);

    connect(m_buttonGroup, &QButtonGroup::buttonToggled,
            this, [this, hiddenLineEdit](QAbstractButton *button, bool checked) {
        if (checked) {
            hiddenLineEdit->setText(button->text());
            emit completeChanged();
        }
    });
}

void SquishToolkitsPage::initializePage()
{
    QTimer::singleShot(0, this, &SquishToolkitsPage::delayedInitialize);
}

bool SquishToolkitsPage::isComplete() const
{
    return m_buttonGroup->checkedButton() != nullptr;
}

bool SquishToolkitsPage::handleReject()
{
    return false;
}

void SquishToolkitsPage::delayedInitialize()
{
    const Utils::FilePath server = settings().squishPath().pathAppended(
                Utils::HostOsInfo::withExecutableSuffix("bin/squishserver"));
    if (server.isExecutableFile())
        fetchServerSettings();
    else
        m_errorLabel->setVisible(true);
}

void SquishToolkitsPage::fetchServerSettings()
{
    auto squishTools = SquishTools::instance();
    QTC_ASSERT(squishTools, return);

    QApplication::setOverrideCursor(Qt::WaitCursor);
    squishTools->queryServerSettings([this](const QString &out, const QString &error) {
        SquishServerSettings s;
        s.setFromXmlOutput(out);
        QApplication::restoreOverrideCursor();
        // FIXME current impl limited to Desktop to avoid confusion and bugreports
        const QStringList ignored = { "Android", "iOS", "VNC", "XView" };
        auto buttons = m_buttonGroup->buttons();
        for (auto button : buttons) {
            const QString text = button->text();
            if (!ignored.contains(text) && s.licensedToolkits.contains(text)) {
                button->setEnabled(true);
                if (s.licensedToolkits.size() == 1)
                    button->setChecked(true);
            }
        }
        m_hiddenLineEdit->setText(s.mappedAuts.keys().join('\n'));

        if (!error.isEmpty()) {
            m_errorLabel->setText(error);
            m_errorLabel->setVisible(true);
        }
    });
}

/********************************* ScriptLanguagePage ********************************************/

SquishScriptLanguagePageFactory::SquishScriptLanguagePageFactory()
{
    setTypeIdsSuffix("SquishScriptLanguage");
}

Utils::WizardPage *SquishScriptLanguagePageFactory::create(ProjectExplorer::JsonWizard *,
                                                           Utils::Id typeId, const QVariant &)
{
    QTC_ASSERT(canCreate(typeId), return nullptr);
    return new SquishScriptLanguagePage;
}

bool SquishScriptLanguagePageFactory::validateData(Utils::Id typeId, const QVariant &, QString *)
{
    QTC_ASSERT(canCreate(typeId), return false);
    return true;
}

SquishScriptLanguagePage::SquishScriptLanguagePage()
{
    setTitle(Tr::tr("Create New Squish Test Suite"));

    auto layout = new QHBoxLayout(this);
    auto groupBox = new QGroupBox(Tr::tr("Available languages:"), this);
    auto buttonLayout = new QVBoxLayout(groupBox);

    auto buttonGroup = new QButtonGroup(this);
    buttonGroup->setExclusive(true);
    const QStringList languages = { "JavaScript", "Perl", "Python", "Ruby", "Tcl" };
    for (const QString &language : languages) {
        auto button = new QRadioButton(language, this);
        button->setChecked(language.startsWith('J'));
        buttonGroup->addButton(button);
        buttonLayout->addWidget(button);
    }
    groupBox->setLayout(buttonLayout);

    layout->addWidget(groupBox);
    auto hiddenLineEdit = new QLineEdit(this);
    hiddenLineEdit->setVisible(false);
    layout->addWidget(hiddenLineEdit);

    connect(buttonGroup, &QButtonGroup::buttonToggled,
            this, [this, hiddenLineEdit](QAbstractButton *button, bool checked) {
        if (checked) {
            hiddenLineEdit->setText(button->text());
            emit completeChanged();
        }
    });
    registerFieldWithName("ChosenLanguage", hiddenLineEdit);
    hiddenLineEdit->setText(buttonGroup->checkedButton()->text());
}

/************************************* AUTPage ***************************************************/

SquishAUTPageFactory::SquishAUTPageFactory()
{
    setTypeIdsSuffix("SquishAUT");
}

Utils::WizardPage *SquishAUTPageFactory::create(ProjectExplorer::JsonWizard *, Utils::Id typeId,
                                                const QVariant &)
{
    QTC_ASSERT(canCreate(typeId), return nullptr);
    return new SquishAUTPage;
}

bool SquishAUTPageFactory::validateData(Utils::Id typeId, const QVariant &, QString *)
{
    QTC_ASSERT(canCreate(typeId), return false);
    return true;
}

SquishAUTPage::SquishAUTPage()
{
    auto layout = new QVBoxLayout(this);
    m_autCombo = new QComboBox(this);
    layout->addWidget(m_autCombo);
    registerFieldWithName("ChosenAUT", m_autCombo, "currentText");
}

void SquishAUTPage::initializePage()
{
    m_autCombo->clear();
    m_autCombo->addItem(Tr::tr("<None>"));
    m_autCombo->addItems(field("RegisteredAUTs").toString().split('\n'));
    m_autCombo->setCurrentIndex(0);
}

/********************************* SquishSuiteGenerator ******************************************/

SquishGeneratorFactory::SquishGeneratorFactory()
{
    setTypeIdsSuffix("SquishSuiteGenerator");
}

ProjectExplorer::JsonWizardGenerator *SquishGeneratorFactory::create(Utils::Id typeId,
                                                                     const QVariant &data,
                                                                     const QString &,
                                                                     Utils::Id,
                                                                     const QVariantMap &)
{
    QTC_ASSERT(canCreate(typeId), return nullptr);

    auto generator = new SquishFileGenerator;
    QString errorMessage;
    generator->setup(data, &errorMessage);

    if (!errorMessage.isEmpty()) {
        qWarning() << "SquishSuiteGenerator setup error:" << errorMessage;
        delete generator;
        return nullptr;
    }
    return generator;
}

bool SquishGeneratorFactory::validateData(Utils::Id typeId, const QVariant &data,
                                          QString *errorMessage)
{
    QTC_ASSERT(canCreate(typeId), return false);

    QScopedPointer<SquishFileGenerator> generator(new SquishFileGenerator);
    return generator->setup(data, errorMessage);
}

bool SquishFileGenerator::setup(const QVariant &data, QString *errorMessage)
{
    if (data.isNull())
        return false;

    if (data.typeId() != QVariant::Map) {
        *errorMessage = Tr::tr("Key is not an object.");
        return false;
    }

    const QVariantMap map = data.toMap();
    auto modeVariant = map.value("mode");
    if (!modeVariant.isValid()) {
        *errorMessage = Tr::tr("Key 'mode' is not set.");
        return false;
    }

    m_mode = modeVariant.toString();
    if (m_mode != "TestSuite") {
        *errorMessage = Tr::tr("Unsupported mode:") + ' ' + m_mode;
        m_mode.clear();
        return false;
    }

    return true;
}

static QString generateSuiteConf(const QString &aut, const QString &language,
                                 const QString &toolkit) {
    QString content;
    content.append("AUT=").append(aut).append('\n');
    content.append("LANGUAGE=").append(language).append('\n');

    // FIXME old format
    content.append("OBJECTMAPSTYLE=script\n");
    // FIXME use what is configured
    content.append("VERSION=3\n");
    content.append("WRAPPERS=").append(toolkit).append('\n');
    return content;
}

Core::GeneratedFiles SquishFileGenerator::fileList(Utils::MacroExpander *expander,
                                                   const Utils::FilePath &wizardDir,
                                                   const Utils::FilePath &projectDir,
                                                   QString *errorMessage)
{
    Q_UNUSED(wizardDir)

    errorMessage->clear();
    // later on differentiate based on m_mode
    QString aut = expander->expand(QString{"%{AUT}"});
    if (aut == Tr::tr("<None>"))
        aut.clear();
    if (aut.contains(' '))
        aut = QString('"' + aut + '"');
    const QString lang = expander->expand(QString{"%{Language}"});
    const QString toolkit = expander->expand(QString{"%{Toolkit}"});;
    const Utils::FilePath suiteConf = projectDir.pathAppended("suite.conf");

    Core::GeneratedFiles result;
    if (expander->expand(QString{"%{VersionControl}"}) == "G.Git") {
        Core::GeneratedFile gitignore(projectDir.pathAppended(".gitignore"));
        const Utils::FilePath orig = Core::ICore::resourcePath()
                .pathAppended("templates/wizards/projects/git.ignore");

        if (QTC_GUARD(orig.exists())) {
            gitignore.setBinaryContents(orig.fileContents().value());
            result.append(gitignore);
        }
    }

    Core::GeneratedFile file(suiteConf);
    file.setAttributes(Core::GeneratedFile::OpenEditorAttribute);
    file.setContents(generateSuiteConf(aut, lang, toolkit));
    result.append(file);
    return result;
}

bool SquishFileGenerator::writeFile(const ProjectExplorer::JsonWizard *,
                                    Core::GeneratedFile *file,
                                    QString *errorMessage)
{
    if (!(file->attributes() & Core::GeneratedFile::CustomGeneratorAttribute)) {
        if (!file->write(errorMessage))
            return false;
    }
    return true;
}

bool SquishFileGenerator::allDone(const ProjectExplorer::JsonWizard *wizard,
                                  Core::GeneratedFile *file, QString *errorMessage)
{
    Q_UNUSED(wizard)
    Q_UNUSED(errorMessage)

    if (m_mode == "TestSuite" && file->filePath().fileName() == "suite.conf") {
        QMetaObject::invokeMethod(SquishFileHandler::instance(), [filePath = file->filePath()] {
            SquishFileHandler::instance()->openTestSuite(filePath);
        }, Qt::QueuedConnection);
    }
    return true;
}


} // namespace Internal
} // namespace Squish
