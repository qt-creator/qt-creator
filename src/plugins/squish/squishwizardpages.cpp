// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "squishwizardpages.h"

#include "squishfilehandler.h"
#include "squishsettings.h"
#include "squishtools.h"
#include "squishtr.h"

#include <coreplugin/icore.h>

#include <projectexplorer/jsonwizard/jsonwizardgeneratorfactory.h>
#include <projectexplorer/jsonwizard/jsonwizardpagefactory.h>

#include <utils/infolabel.h>
#include <utils/qtcassert.h>
#include <utils/wizardpage.h>

#include <QApplication>
#include <QButtonGroup>
#include <QComboBox>
#include <QGroupBox>
#include <QLineEdit>
#include <QRadioButton>
#include <QTimer>
#include <QVBoxLayout>

using namespace ProjectExplorer;
using namespace Utils;

namespace Squish::Internal {

/************************************ ToolkitsPage ***********************************************/

class SquishToolkitsPage final : public WizardPage
{
public:
    SquishToolkitsPage();

    void initializePage() final;
    bool isComplete() const final;
    bool handleReject() final;

private:
    void delayedInitialize();
    void fetchServerSettings();

    QButtonGroup *m_buttonGroup = nullptr;
    QLineEdit *m_hiddenLineEdit = nullptr;
    InfoLabel *m_errorLabel = nullptr;
};

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

    m_errorLabel = new InfoLabel(Tr::tr("Invalid Squish settings. Configure Squish "
                                        "installation path inside "
                                        "Preferences... > Squish > General to use "
                                        "this wizard."),
                                        InfoLabel::Error, this);
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
    const FilePath server = settings().squishPath().pathAppended(
                HostOsInfo::withExecutableSuffix("bin/squishserver"));
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
        const QList<QAbstractButton*> buttons = m_buttonGroup->buttons();
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

class SquishToolkitsPageFactory final : public JsonWizardPageFactory
{
public:
    SquishToolkitsPageFactory()
    {
        setTypeIdsSuffix("SquishToolkits");
    }

    WizardPage *create(JsonWizard *wizard, Id typeId, const QVariant &data) final
    {
        Q_UNUSED(wizard)
        Q_UNUSED(data)
        QTC_ASSERT(canCreate(typeId), return nullptr);
        return new SquishToolkitsPage;
    }

    bool validateData(Id typeId, const QVariant &data, QString *errorMessage) final
    {
        Q_UNUSED(data)
        Q_UNUSED(errorMessage)
        QTC_ASSERT(canCreate(typeId), return false);
        return true;
    }
};

/********************************* ScriptLanguagePage ********************************************/

class SquishScriptLanguagePage final : public WizardPage
{
public:
    SquishScriptLanguagePage()
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
};

class SquishScriptLanguagePageFactory final : public JsonWizardPageFactory
{
public:
    SquishScriptLanguagePageFactory()
    {
        setTypeIdsSuffix("SquishScriptLanguage");
    }

    WizardPage *create(JsonWizard *wizard, Id typeId, const QVariant &data) final
    {
        Q_UNUSED(wizard)
        Q_UNUSED(data)
        QTC_ASSERT(canCreate(typeId), return nullptr);
        return new SquishScriptLanguagePage;
    }
    bool validateData(Id typeId, const QVariant &, QString *) final
    {
        QTC_ASSERT(canCreate(typeId), return false);
        return true;
    }
};


/************************************* AUTPage ***************************************************/

class SquishAUTPage final : public WizardPage
{
public:
    SquishAUTPage()
    {
        auto layout = new QVBoxLayout(this);
        m_autCombo = new QComboBox(this);
        layout->addWidget(m_autCombo);
        registerFieldWithName("ChosenAUT", m_autCombo, "currentText");
    }

    void initializePage() final;

private:
    QComboBox *m_autCombo = nullptr;
};

void SquishAUTPage::initializePage()
{
    m_autCombo->clear();
    m_autCombo->addItem(Tr::tr("<None>"));
    m_autCombo->addItems(field("RegisteredAUTs").toString().split('\n'));
    m_autCombo->setCurrentIndex(0);
}

class SquishAUTPageFactory final : public JsonWizardPageFactory
{
public:
    SquishAUTPageFactory()
    {
        setTypeIdsSuffix("SquishAUT");
    }

    WizardPage *create(JsonWizard *wizard, Id typeId, const QVariant &data) final
    {
        Q_UNUSED(wizard)
        Q_UNUSED(data);
        QTC_ASSERT(canCreate(typeId), return nullptr);
        return new SquishAUTPage;
    }

    bool validateData(Id typeId, const QVariant &data, QString *errorMessage) final
    {
        Q_UNUSED(data)
        Q_UNUSED(errorMessage)
        QTC_ASSERT(canCreate(typeId), return false);
        return true;
    }
};

/********************************* SquishSuiteGenerator ******************************************/

class SquishFileGenerator final : public JsonWizardGenerator
{
public:
    Result<> setup(const QVariant &data);
    Core::GeneratedFiles fileList(MacroExpander *expander,
                                  const FilePath &wizardDir,
                                  const FilePath &projectDir,
                                  QString *errorMessage) final;
    Result<> writeFile(const ProjectExplorer::JsonWizard *wizard, Core::GeneratedFile *file) final;
    Result<> allDone(const ProjectExplorer::JsonWizard *wizard, Core::GeneratedFile *file) final;

private:
    QString m_mode;
};

Result<> SquishFileGenerator::setup(const QVariant &data)
{
    if (data.isNull())
        return ResultError("No data");

    if (data.typeId() != QMetaType::QVariantMap)
        return ResultError(Tr::tr("Key is not an object."));

    const QVariantMap map = data.toMap();
    const QVariant modeVariant = map.value("mode");
    if (!modeVariant.isValid())
        return ResultError(Tr::tr("Key 'mode' is not set."));

    m_mode = modeVariant.toString();
    if (m_mode != "TestSuite") {
        const Result<> res = ResultError(Tr::tr("Unsupported mode:") + ' ' + m_mode);
        m_mode.clear();
        return res;
    }

    return ResultOk;
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

Core::GeneratedFiles SquishFileGenerator::fileList(MacroExpander *expander,
                                                   const FilePath &wizardDir,
                                                   const FilePath &projectDir,
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
    const FilePath suiteConf = projectDir.pathAppended("suite.conf");

    Core::GeneratedFiles result;
    if (expander->expand(QString{"%{VersionControl}"}) == "G.Git") {
        Core::GeneratedFile gitignore(projectDir.pathAppended(".gitignore"));
        const FilePath orig = Core::ICore::resourcePath()
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

Result<> SquishFileGenerator::writeFile(const JsonWizard *, Core::GeneratedFile *file)
{
    if (file->attributes() & Core::GeneratedFile::CustomGeneratorAttribute)
        return ResultOk;
    return file->write();
}

Result<> SquishFileGenerator::allDone(const JsonWizard *wizard, Core::GeneratedFile *file)
{
    Q_UNUSED(wizard)

    if (m_mode == "TestSuite" && file->filePath().fileName() == "suite.conf") {
        QMetaObject::invokeMethod(SquishFileHandler::instance(), [filePath = file->filePath()] {
            SquishFileHandler::instance()->openTestSuite(filePath);
        }, Qt::QueuedConnection);
    }
    return ResultOk;
}

void setupSquishWizardPages()
{
    static SquishToolkitsPageFactory theSquishToolkitsPageFactory;
    static SquishScriptLanguagePageFactory theSquishScriptLanguagePageFactory;
    static SquishAUTPageFactory theSquishAUTPageFactory;
    static JsonWizardGeneratorTypedFactory<SquishFileGenerator>
        theSquishGeneratorFactory("SquishSuiteGenerator");
}

} // Squish::Internal
