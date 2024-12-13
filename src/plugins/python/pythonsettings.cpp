// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "pythonsettings.h"

#include "pythonconstants.h"
#include "pythonkitaspect.h"
#include "pythontr.h"
#include "pythonutils.h"

#include <coreplugin/dialogs/ioptionspage.h>
#include <coreplugin/icore.h>
#include <coreplugin/progressmanager/processprogress.h>

#include <projectexplorer/kitaspect.h>
#include <projectexplorer/environmentkitaspect.h>
#include <projectexplorer/kitmanager.h>

#include <extensionsystem/pluginmanager.h>

#include <languageclient/languageclient_global.h>
#include <languageclient/languageclientsettings.h>
#include <languageclient/languageclientmanager.h>

#include <texteditor/textdocument.h>
#include <texteditor/texteditor.h>

#include <utils/algorithm.h>
#include <utils/async.h>
#include <utils/detailswidget.h>
#include <utils/environment.h>
#include <utils/layoutbuilder.h>
#include <utils/listmodel.h>
#include <utils/pathchooser.h>
#include <utils/qtcprocess.h>
#include <utils/qtcassert.h>
#include <utils/treemodel.h>
#include <utils/utilsicons.h>

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QFormLayout>
#include <QGroupBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QPointer>
#include <QPushButton>
#include <QSettings>
#include <QStackedWidget>
#include <QTreeView>
#include <QVBoxLayout>
#include <QWidget>

using namespace Layouting;
using namespace ProjectExplorer;
using namespace Utils;

namespace Python::Internal {

static Interpreter createInterpreter(const FilePath &python,
                                     const QString &defaultName,
                                     const QString &suffix = {})
{
    Interpreter result;
    result.id = QUuid::createUuid().toString();
    result.command = python;

    Process pythonProcess;
    pythonProcess.setProcessChannelMode(QProcess::MergedChannels);
    pythonProcess.setCommand({python, {"--version"}});
    using namespace std::chrono_literals;
    pythonProcess.runBlocking(1s);
    if (pythonProcess.result() == ProcessResult::FinishedWithSuccess)
        result.name = pythonProcess.cleanedStdOut().trimmed();
    if (result.name.isEmpty())
        result.name = defaultName;
    QDir pythonDir(python.parentDir().toString());
    if (pythonDir.exists() && pythonDir.exists("activate") && pythonDir.cdUp())
        result.name += QString(" (%1)").arg(pythonDir.dirName());
    if (!suffix.isEmpty())
        result.name += ' ' + suffix;

    return result;
}

class InterpreterDetailsWidget : public QWidget
{
    Q_OBJECT
public:
    InterpreterDetailsWidget(QWidget *parent)
        : QWidget(parent)
        , m_name(new QLineEdit)
        , m_executable(new PathChooser())
    {
        m_executable->setExpectedKind(PathChooser::ExistingCommand);
        m_executable->setAllowPathFromDevice(true);

        connect(m_name, &QLineEdit::textChanged, this, &InterpreterDetailsWidget::changed);
        connect(m_executable, &PathChooser::textChanged, this, &InterpreterDetailsWidget::changed);

        Form {
            Tr::tr("Name:"), m_name, br,
            Tr::tr("Executable"), m_executable,
            noMargin
        }.attachTo(this);
    }

    void updateInterpreter(const Interpreter &interpreter)
    {
        QSignalBlocker blocker(this); // do not emit changed when we change the controls here
        m_currentInterpreter = interpreter;
        m_name->setText(interpreter.name);
        m_executable->setFilePath(interpreter.command);
    }

    Interpreter toInterpreter()
    {
        m_currentInterpreter.command = m_executable->filePath();
        m_currentInterpreter.name = m_name->text();
        return m_currentInterpreter;
    }
    QLineEdit *m_name = nullptr;
    PathChooser *m_executable = nullptr;
    Interpreter m_currentInterpreter;

signals:
    void changed();
};


class InterpreterOptionsWidget : public Core::IOptionsPageWidget
{
public:
    InterpreterOptionsWidget();

    void apply() override;

    void addInterpreter(const Interpreter &interpreter);
    void removeInterpreterFrom(const QString &detectionSource);
    QList<Interpreter> interpreters() const;
    QList<Interpreter> interpreterFrom(const QString &detectionSource) const;

private:
    QTreeView *m_view = nullptr;
    ListModel<Interpreter> * const m_model;
    InterpreterDetailsWidget *m_detailsWidget = nullptr;
    QPushButton *m_deleteButton = nullptr;
    QPushButton *m_makeDefaultButton = nullptr;
    QPushButton *m_generateKitButton = nullptr;
    QPushButton *m_cleanButton = nullptr;
    QString m_defaultId;

    void currentChanged(const QModelIndex &index, const QModelIndex &previous);
    void detailsChanged();
    void updateCleanButton();
    void updateGenerateKitButton(const Interpreter &interpreter);
    void addItem();
    void deleteItem();
    void makeDefault();
    void generateKit();
    void cleanUp();
};

InterpreterOptionsWidget::InterpreterOptionsWidget()
    : m_model(createInterpreterModel(this))
    , m_detailsWidget(new InterpreterDetailsWidget(this))
    , m_defaultId(PythonSettings::defaultInterpreter().id)
{
    auto addButton = new QPushButton(Tr::tr("&Add"), this);

    m_deleteButton = new QPushButton(Tr::tr("&Delete"), this);
    m_deleteButton->setEnabled(false);
    m_makeDefaultButton = new QPushButton(Tr::tr("&Make Default"));
    m_makeDefaultButton->setEnabled(false);
    m_generateKitButton = new QPushButton(Tr::tr("&Generate Kit"));
    m_generateKitButton->setEnabled(false);

    m_cleanButton = new QPushButton(Tr::tr("&Clean Up"), this);
    m_cleanButton->setToolTip(Tr::tr("Remove all Python interpreters without a valid executable."));

    m_view = new QTreeView(this);

    Column buttons {
        addButton,
        m_deleteButton,
        m_makeDefaultButton,
        m_generateKitButton,
        m_cleanButton,
        st
    };

    Column {
        Row { m_view, buttons },
        m_detailsWidget
    }.attachTo(this);

    updateCleanButton();

    m_detailsWidget->hide();

    m_view->setModel(m_model);
    m_view->setHeaderHidden(true);
    m_view->setSelectionMode(QAbstractItemView::SingleSelection);
    m_view->setSelectionBehavior(QAbstractItemView::SelectItems);

    connect(addButton, &QPushButton::pressed, this, &InterpreterOptionsWidget::addItem);
    connect(m_deleteButton, &QPushButton::pressed, this, &InterpreterOptionsWidget::deleteItem);
    connect(m_makeDefaultButton, &QPushButton::pressed, this, &InterpreterOptionsWidget::makeDefault);
    connect(m_generateKitButton, &QPushButton::pressed, this, &InterpreterOptionsWidget::generateKit);
    connect(m_cleanButton, &QPushButton::pressed, this, &InterpreterOptionsWidget::cleanUp);

    connect(m_detailsWidget, &InterpreterDetailsWidget::changed,
            this, &InterpreterOptionsWidget::detailsChanged);
    connect(m_view->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &InterpreterOptionsWidget::currentChanged);
}

void InterpreterOptionsWidget::apply()
{
    PythonSettings::setInterpreter(interpreters(), m_defaultId);
}

void InterpreterOptionsWidget::addInterpreter(const Interpreter &interpreter)
{
    m_model->appendItem(interpreter);
}

void InterpreterOptionsWidget::removeInterpreterFrom(const QString &detectionSource)
{
    m_model->destroyItems(Utils::equal(&Interpreter::detectionSource, detectionSource));
}

QList<Interpreter> InterpreterOptionsWidget::interpreters() const
{
    QList<Interpreter> interpreters;
    for (const TreeItem *treeItem : *m_model)
        interpreters << static_cast<const ListItem<Interpreter> *>(treeItem)->itemData;
    return interpreters;
}

QList<Interpreter> InterpreterOptionsWidget::interpreterFrom(const QString &detectionSource) const
{
    return m_model->allData(Utils::equal(&Interpreter::detectionSource, detectionSource));
}

void InterpreterOptionsWidget::currentChanged(const QModelIndex &index, const QModelIndex &previous)
{
    if (previous.isValid()) {
        m_model->itemAt(previous.row())->itemData = m_detailsWidget->toInterpreter();
        emit m_model->dataChanged(previous, previous);
    }
    if (index.isValid()) {
        const Interpreter interpreter = m_model->itemAt(index.row())->itemData;
        m_detailsWidget->updateInterpreter(interpreter);
        m_detailsWidget->show();
        updateGenerateKitButton(interpreter);
    } else {
        m_detailsWidget->hide();
        m_generateKitButton->setEnabled(false);
    }
    m_deleteButton->setEnabled(index.isValid());
    m_makeDefaultButton->setEnabled(index.isValid());
}

void InterpreterOptionsWidget::detailsChanged()
{
    const QModelIndex &index = m_view->currentIndex();
    if (index.isValid()) {
        const Interpreter interpreter = m_detailsWidget->toInterpreter();
        m_model->itemAt(index.row())->itemData = interpreter;
        emit m_model->dataChanged(index, index);
        updateGenerateKitButton(interpreter);
    }
    updateCleanButton();
}

void InterpreterOptionsWidget::updateCleanButton()
{
    m_cleanButton->setEnabled(Utils::anyOf(m_model->allData(), [](const Interpreter &interpreter) {
        return !interpreter.command.isExecutableFile();
    }));
}

void InterpreterOptionsWidget::updateGenerateKitButton(const Interpreter &interpreter)
{
    bool enabled = !KitManager::kit(Id::fromString(interpreter.id))
            && (!interpreter.command.isLocal() || interpreter.command.isExecutableFile());
    m_generateKitButton->setEnabled(enabled);
}

void InterpreterOptionsWidget::addItem()
{
    const QModelIndex &index = m_model->indexForItem(m_model->appendItem(
        {QUuid::createUuid().toString(), QString("Python"), FilePath(), false}));
    QTC_ASSERT(index.isValid(), return);
    m_view->setCurrentIndex(index);
    updateCleanButton();
}

void InterpreterOptionsWidget::deleteItem()
{
    const QModelIndex &index = m_view->currentIndex();
    if (index.isValid())
        m_model->destroyItem(m_model->itemAt(index.row()));
    updateCleanButton();
}

class InterpreterOptionsPage : public Core::IOptionsPage
{
public:
    InterpreterOptionsPage()
    {
        setId(Constants::C_PYTHONOPTIONS_PAGE_ID);
        setDisplayName(Tr::tr("Interpreters"));
        setCategory(Constants::C_PYTHON_SETTINGS_CATEGORY);
        setWidgetCreator([] { return new InterpreterOptionsWidget(); });
    }

    QList<Interpreter> interpreters()
    {
        return static_cast<InterpreterOptionsWidget *>(widget())->interpreters();
    }

    void addInterpreter(const Interpreter &interpreter)
    {
        static_cast<InterpreterOptionsWidget *>(widget())->addInterpreter(interpreter);
    }

    void removeInterpreterFrom(const QString &detectionSource)
    {
        static_cast<InterpreterOptionsWidget *>(widget())->removeInterpreterFrom(detectionSource);
    }

    QList<Interpreter> interpreterFrom(const QString &detectionSource)
    {
        return static_cast<InterpreterOptionsWidget *>(widget())->interpreterFrom(detectionSource);
    }

    QStringList keywords() const final
    {
        return {
            Tr::tr("Name:"),
            Tr::tr("Executable"),
            Tr::tr("&Add"),
            Tr::tr("&Delete"),
            Tr::tr("&Clean Up"),
            Tr::tr("&Make Default")
        };
    }
};

static InterpreterOptionsPage &interpreterOptionsPage()
{
    static InterpreterOptionsPage page;
    return page;
}

static const QStringList &plugins()
{
    static const QStringList plugins{"flake8",
                                     "jedi_completion",
                                     "jedi_definition",
                                     "jedi_hover",
                                     "jedi_references",
                                     "jedi_signature_help",
                                     "jedi_symbols",
                                     "mccabe",
                                     "pycodestyle",
                                     "pydocstyle",
                                     "pyflakes",
                                     "pylint",
                                     "yapf"};
    return plugins;
}

class PyLSConfigureWidget : public Core::IOptionsPageWidget
{
public:
    PyLSConfigureWidget()
        : m_editor(LanguageClient::createJsonEditor(this))
        , m_advancedLabel(new QLabel)
        , m_pluginsGroup(new QGroupBox(Tr::tr("Plugins:")))
        , m_mainGroup(new QGroupBox(Tr::tr("Use Python Language Server")))
    {
        m_mainGroup->setCheckable(true);

        auto mainGroupLayout = new QVBoxLayout;

        auto pluginsLayout = new QVBoxLayout;
        m_pluginsGroup->setLayout(pluginsLayout);
        m_pluginsGroup->setFlat(true);
        for (const QString &plugin : plugins()) {
            auto checkBox = new QCheckBox(plugin, this);
            connect(checkBox, &QCheckBox::clicked, this, [this, plugin, checkBox]() {
                updatePluginEnabled(checkBox->checkState(), plugin);
            });
            m_checkBoxes[plugin] = checkBox;
            pluginsLayout->addWidget(checkBox);
        }

        mainGroupLayout->addWidget(m_pluginsGroup);

        const QString labelText = Tr::tr("For a complete list of available options, consult the "
                                         "[Python LSP Server configuration documentation](%1).")
                                      .arg("https://github.com/python-lsp/python-lsp-server/blob/"
                                           "develop/CONFIGURATION.md");
        m_advancedLabel->setTextFormat(Qt::MarkdownText);
        m_advancedLabel->setText(labelText);
        m_advancedLabel->setOpenExternalLinks(true);
        mainGroupLayout->addWidget(m_advancedLabel);
        mainGroupLayout->addWidget(m_editor->editorWidget(), 1);

        mainGroupLayout->addStretch();

        auto advanced = new QCheckBox(Tr::tr("Advanced"));
        advanced->setChecked(false);
        mainGroupLayout->addWidget(advanced);

        m_mainGroup->setLayout(mainGroupLayout);

        QVBoxLayout *mainLayout = new QVBoxLayout;
        mainLayout->addWidget(m_mainGroup);
        setLayout(mainLayout);

        m_editor->textDocument()->setPlainText(PythonSettings::pylsConfiguration());
        m_mainGroup->setChecked(PythonSettings::pylsEnabled());
        updateCheckboxes();

        setAdvanced(false);

        connect(advanced,
                &QCheckBox::toggled,
                this,
                &PyLSConfigureWidget::setAdvanced);

    }

    void apply() override
    {
        PythonSettings::setPylsEnabled(m_mainGroup->isChecked());
        PythonSettings::setPyLSConfiguration(m_editor->textDocument()->plainText());
    }
private:
    void setAdvanced(bool advanced)
    {
        m_editor->editorWidget()->setVisible(advanced);
        m_advancedLabel->setVisible(advanced);
        m_pluginsGroup->setVisible(!advanced);
        updateCheckboxes();
    }

    void updateCheckboxes()
    {
        const QJsonDocument document = QJsonDocument::fromJson(
            m_editor->textDocument()->plainText().toUtf8());
        if (document.isObject()) {
            const QJsonObject pluginsObject
                = document.object()["pylsp"].toObject()["plugins"].toObject();
            for (const QString &plugin : plugins()) {
                auto checkBox = m_checkBoxes[plugin];
                if (!checkBox)
                    continue;
                const QJsonValue enabled = pluginsObject[plugin].toObject()["enabled"];
                if (!enabled.isBool())
                    checkBox->setCheckState(Qt::PartiallyChecked);
                else
                    checkBox->setCheckState(enabled.toBool(false) ? Qt::Checked : Qt::Unchecked);
            }
        }
    }

    void updatePluginEnabled(Qt::CheckState check, const QString &plugin)
    {
        if (check == Qt::PartiallyChecked)
            return;
        QJsonDocument document = QJsonDocument::fromJson(
            m_editor->textDocument()->plainText().toUtf8());
        QJsonObject config;
        if (!document.isNull())
            config = document.object();
        QJsonObject pylsp = config["pylsp"].toObject();
        QJsonObject plugins = pylsp["plugins"].toObject();
        QJsonObject pluginValue = plugins[plugin].toObject();
        pluginValue.insert("enabled", check == Qt::Checked);
        plugins.insert(plugin, pluginValue);
        pylsp.insert("plugins", plugins);
        config.insert("pylsp", pylsp);
        document.setObject(config);
        m_editor->textDocument()->setPlainText(QString::fromUtf8(document.toJson()));
    }

    QMap<QString, QCheckBox *> m_checkBoxes;
    TextEditor::BaseTextEditor *m_editor = nullptr;
    QLabel *m_advancedLabel = nullptr;
    QGroupBox *m_pluginsGroup = nullptr;
    QGroupBox *m_mainGroup = nullptr;
};


class PyLSOptionsPage : public Core::IOptionsPage
{
public:
    PyLSOptionsPage()
    {
        setId(Constants::C_PYLSCONFIGURATION_PAGE_ID);
        setDisplayName(Tr::tr("Language Server Configuration"));
        setCategory(Constants::C_PYTHON_SETTINGS_CATEGORY);
        setWidgetCreator([]() {return new PyLSConfigureWidget();});
    }
};

static PyLSOptionsPage &pylspOptionsPage()
{
    static PyLSOptionsPage page;
    return page;
}

void InterpreterOptionsWidget::makeDefault()
{
    const QModelIndex &index = m_view->currentIndex();
    if (index.isValid()) {
        QModelIndex defaultIndex = m_model->findIndex([this](const Interpreter &interpreter) {
            return interpreter.id == m_defaultId;
        });
        m_defaultId = m_model->itemAt(index.row())->itemData.id;
        emit m_model->dataChanged(index, index, {Qt::FontRole});
        if (defaultIndex.isValid())
            emit m_model->dataChanged(defaultIndex, defaultIndex, {Qt::FontRole});
    }
}

void InterpreterOptionsWidget::generateKit()
{
    const QModelIndex &index = m_view->currentIndex();
    if (index.isValid())
        PythonSettings::addKitsForInterpreter(m_model->itemAt(index.row())->itemData, true);
    m_generateKitButton->setEnabled(false);
}

void InterpreterOptionsWidget::cleanUp()
{
    m_model->destroyItems(
        [](const Interpreter &interpreter) { return !interpreter.command.isExecutableFile(); });
    updateCleanButton();
}

constexpr char settingsGroupKey[] = "Python";
constexpr char interpreterKey[] = "Interpeter";
constexpr char defaultKey[] = "DefaultInterpeter";
constexpr char kitsGeneratedKey[] = "KitsGenerated";
constexpr char pylsEnabledKey[] = "PylsEnabled";
constexpr char pylsConfigurationKey[] = "PylsConfiguration";

static QString defaultPylsConfiguration()
{
    static QJsonObject configuration;
    if (configuration.isEmpty()) {
        QJsonObject enabled;
        enabled.insert("enabled", true);
        QJsonObject disabled;
        disabled.insert("enabled", false);
        QJsonObject plugins;
        plugins.insert("flake8", disabled);
        plugins.insert("jedi_completion", enabled);
        plugins.insert("jedi_definition", enabled);
        plugins.insert("jedi_hover", enabled);
        plugins.insert("jedi_references", enabled);
        plugins.insert("jedi_signature_help", enabled);
        plugins.insert("jedi_symbols", enabled);
        plugins.insert("mccabe", disabled);
        plugins.insert("pycodestyle", disabled);
        plugins.insert("pydocstyle", disabled);
        plugins.insert("pyflakes", enabled);
        plugins.insert("pylint", disabled);
        plugins.insert("yapf", enabled);
        QJsonObject pylsp;
        pylsp.insert("plugins", plugins);
        configuration.insert("pylsp", pylsp);
    }
    return QString::fromUtf8(QJsonDocument(configuration).toJson());
}

void PythonSettings::disableOutdatedPylsNow()
{
    using namespace LanguageClient;
    const QList<BaseSettings *>
            settings = LanguageClientSettings::pageSettings();
    for (const BaseSettings *setting : settings) {
        if (setting->m_settingsTypeId != LanguageClient::Constants::LANGUAGECLIENT_STDIO_SETTINGS_ID)
            continue;
        auto stdioSetting = static_cast<const StdIOSettings *>(setting);
        if (stdioSetting->arguments().startsWith("-m pyls")
                && stdioSetting->m_languageFilter.isSupported("foo.py", Constants::C_PY_MIMETYPE)) {
            LanguageClientManager::enableClientSettings(stdioSetting->m_id, false);
        }
    }
}

void PythonSettings::disableOutdatedPyls()
{
    using namespace ExtensionSystem;
    if (PluginManager::isInitializationDone()) {
        disableOutdatedPylsNow();
    } else {
        QObject::connect(PluginManager::instance(), &PluginManager::initializationDone,
                         this, &PythonSettings::disableOutdatedPylsNow);
    }
}

static void pythonsFromRegistry(QPromise<QList<Interpreter>> &promise)
{
    QList<Interpreter> pythons;
    QSettings pythonRegistry("HKEY_LOCAL_MACHINE\\SOFTWARE\\Python\\PythonCore",
                             QSettings::NativeFormat);
    for (const QString &versionGroup : pythonRegistry.childGroups()) {
        if (promise.isCanceled())
            return;

        pythonRegistry.beginGroup(versionGroup);
        QString name = pythonRegistry.value("DisplayName").toString();
        QVariant regVal = pythonRegistry.value("InstallPath/ExecutablePath");
        if (regVal.isValid()) {
            const FilePath &executable = FilePath::fromUserInput(regVal.toString());
            if (executable.exists()) {
                pythons << Interpreter{QUuid::createUuid().toString(),
                                       name,
                                       FilePath::fromUserInput(regVal.toString())};
            }
        }
        regVal = pythonRegistry.value("InstallPath/.");
        if (regVal.isValid()) {
            const FilePath &path = FilePath::fromUserInput(regVal.toString());
            const FilePath python = path.pathAppended("python").withExecutableSuffix();
            if (python.exists())
                pythons << createInterpreter(python, "Python " + versionGroup);
        }
        pythonRegistry.endGroup();
    }
    promise.addResult(pythons);
}

static void pythonsFromPath(QPromise<QList<Interpreter>> &promise)
{
    QList<Interpreter> pythons;
    if (HostOsInfo::isWindowsHost()) {
        for (const FilePath &executable : FilePath("python").searchAllInPath()) {
            if (promise.isCanceled())
                return;

            // Windows creates empty redirector files that may interfere
            if (executable.toFileInfo().size() == 0)
                continue;
            if (executable.exists())
                pythons << createInterpreter(executable, "Python from Path");
        }
    } else {
        const QStringList filters = {"python",
                                     "python[1-9].[0-9]",
                                     "python[1-9].[1-9][0-9]",
                                     "python[1-9]"};
        const FilePaths dirs = Environment::systemEnvironment().path();
        QSet<FilePath> used;
        for (const FilePath &path : dirs) {
            const QDir dir(path.toString());
            for (const QFileInfo &fi : dir.entryInfoList(filters)) {
                if (promise.isCanceled())
                    return;

                const FilePath executable = FilePath::fromUserInput(fi.canonicalFilePath());
                if (!used.contains(executable) && executable.exists()) {
                    used.insert(executable);
                    pythons << createInterpreter(executable, "Python from Path");
                }
            }
        }
    }
    promise.addResult(pythons);
}

static QString idForPythonFromPath(const QList<Interpreter> &pythons)
{
    FilePath pythonFromPath = FilePath("python3").searchInPath();
    if (pythonFromPath.isEmpty())
        pythonFromPath = FilePath("python").searchInPath();
    if (pythonFromPath.isEmpty())
        return {};
    const Interpreter &defaultInterpreter
        = findOrDefault(pythons, [pythonFromPath](const Interpreter &interpreter) {
              return interpreter.command == pythonFromPath;
          });
    return defaultInterpreter.id;
}

static PythonSettings *settingsInstance = nullptr;

static bool alreadyRegistered(const Interpreter &candidate)
{
    return Utils::anyOf(settingsInstance->interpreters(),
                        [candidate = candidate.command](const Interpreter &interpreter) {
                            return interpreter.command.isSameDevice(candidate)
                                   && interpreter.command.resolveSymlinks()
                                          == candidate.resolveSymlinks();
                        });
}

PythonSettings::PythonSettings()
{
    QTC_ASSERT(!settingsInstance, return);
    settingsInstance = this;

    setObjectName("PythonSettings");
    ExtensionSystem::PluginManager::addObject(this);

    initFromSettings(Core::ICore::settings());

    const auto onRegistrySetup = [](Async<QList<Interpreter>> &task) {
        task.setConcurrentCallData(pythonsFromRegistry);
    };
    const auto onPathSetup = [](Async<QList<Interpreter>> &task) {
        task.setConcurrentCallData(pythonsFromPath);
    };
    const auto onTaskDone = [](const Async<QList<Interpreter>> &task) {
        if (!task.isResultAvailable())
            return;

        const auto interpreters = task.result();
        for (const Interpreter &interpreter : interpreters) {
            if (!alreadyRegistered(interpreter))
                settingsInstance->addInterpreter(interpreter);
        }
    };

    const Tasking::Group recipe {
        Tasking::finishAllAndSuccess,
        Utils::HostOsInfo::isWindowsHost()
            ? AsyncTask<QList<Interpreter>>(onRegistrySetup, onTaskDone) : Tasking::nullItem,
        AsyncTask<QList<Interpreter>>(onPathSetup, onTaskDone)
    };
    m_taskTreeRunner.start(recipe);

    if (m_defaultInterpreterId.isEmpty())
        m_defaultInterpreterId = idForPythonFromPath(m_interpreters);

    writeToSettings(Core::ICore::settings());

    interpreterOptionsPage();
    pylspOptionsPage();
}

PythonSettings::~PythonSettings()
{
    ExtensionSystem::PluginManager::removeObject(this);
    settingsInstance = nullptr;
}

static void setRelevantAspectsToKit(Kit *k)
{
    QTC_ASSERT(k, return);
    QSet<Utils::Id> relevantAspects = k->relevantAspects();
    relevantAspects.unite({PythonKitAspect::id(), EnvironmentKitAspect::id()});
    k->setRelevantAspects(relevantAspects);
}

void PythonSettings::addKitsForInterpreter(const Interpreter &interpreter, bool force)
{
    if (!KitManager::isLoaded()) {
        connect(KitManager::instance(),
                &KitManager::kitsLoaded,
                settingsInstance,
                [interpreter, force]() { addKitsForInterpreter(interpreter, force); });
        return;
    }

    const Id kitId = Id::fromString(interpreter.id);
    if (Kit *k = KitManager::kit(kitId)) {
        setRelevantAspectsToKit(k);
    } else if (force || !isVenvPython(interpreter.command)) {
        KitManager::registerKit(
            [interpreter](Kit *k) {
                k->setAutoDetected(true);
                k->setAutoDetectionSource("Python");
                k->setUnexpandedDisplayName("%{Python:Name}");
                setRelevantAspectsToKit(k);
                PythonKitAspect::setPython(k, interpreter.id);
                k->setSticky(PythonKitAspect::id(), true);
            },
            kitId);
    }
}

void PythonSettings::removeKitsForInterpreter(const Interpreter &interpreter)
{
    if (!KitManager::isLoaded()) {
        connect(KitManager::instance(), &KitManager::kitsLoaded, settingsInstance, [interpreter]() {
            removeKitsForInterpreter(interpreter);
        });
        return;
    }

    if (Kit *k = KitManager::kit(Id::fromString(interpreter.id)))
        KitManager::deregisterKit(k);
}

bool PythonSettings::interpreterIsValid(const Interpreter &interpreter)
{
    return !interpreter.command.isLocal() || interpreter.command.isExecutableFile();
}

void PythonSettings::setInterpreter(const QList<Interpreter> &interpreters, const QString &defaultId)
{
    if (defaultId == settingsInstance->m_defaultInterpreterId
        && interpreters == settingsInstance->m_interpreters) {
        return;
    }
    QList<Interpreter> toRemove = settingsInstance->m_interpreters;
    for (const Interpreter &interpreter : interpreters) {
        if (!Utils::eraseOne(toRemove, Utils::equal(&Interpreter::id, interpreter.id)))
            addKitsForInterpreter(interpreter, false);
    }
    for (const Interpreter &interpreter : toRemove)
        removeKitsForInterpreter(interpreter);
    settingsInstance->m_interpreters = interpreters;
    settingsInstance->m_defaultInterpreterId = defaultId;
    saveSettings();
}

void PythonSettings::setPyLSConfiguration(const QString &configuration)
{
    if (configuration == settingsInstance->m_pylsConfiguration)
        return;
    settingsInstance->m_pylsConfiguration = configuration;
    saveSettings();
    emit instance()->pylsConfigurationChanged(configuration);
}

void PythonSettings::setPylsEnabled(const bool &enabled)
{
    if (enabled == settingsInstance->m_pylsEnabled)
        return;
    settingsInstance->m_pylsEnabled = enabled;
    saveSettings();
    emit instance()->pylsEnabledChanged(enabled);
}

bool PythonSettings::pylsEnabled()
{
    return settingsInstance->m_pylsEnabled;
}

QString PythonSettings::pylsConfiguration()
{
    return settingsInstance->m_pylsConfiguration;
}

void PythonSettings::addInterpreter(const Interpreter &interpreter, bool isDefault)
{
    if (Utils::anyOf(settingsInstance->m_interpreters, Utils::equal(&Interpreter::id, interpreter.id)))
        return;
    settingsInstance->m_interpreters.append(interpreter);
    if (isDefault)
        settingsInstance->m_defaultInterpreterId = interpreter.id;
    saveSettings();
    addKitsForInterpreter(interpreter, false);
}

Interpreter PythonSettings::addInterpreter(const FilePath &interpreterPath,
                                           bool isDefault,
                                           const QString &nameSuffix)
{
    const Interpreter interpreter = createInterpreter(interpreterPath, {}, nameSuffix);
    addInterpreter(interpreter, isDefault);
    return interpreter;
}

PythonSettings *PythonSettings::instance()
{
    QTC_CHECK(settingsInstance);
    return settingsInstance;
}

void PythonSettings::createVirtualEnvironmentInteractive(
    const FilePath &startDirectory,
    const Interpreter &defaultInterpreter,
    const std::function<void(const FilePath &)> &callback)
{
    QDialog dialog;
    dialog.setModal(true);
    auto layout = new QFormLayout(&dialog);
    auto interpreters = new QComboBox;
    const QString preselectedId = defaultInterpreter.id.isEmpty()
                                      ? PythonSettings::defaultInterpreter().id
                                      : defaultInterpreter.id;
    for (const Interpreter &interpreter : PythonSettings::interpreters()) {
        interpreters->addItem(interpreter.name, interpreter.id);
        if (!preselectedId.isEmpty() && interpreter.id == preselectedId)
            interpreters->setCurrentIndex(interpreters->count() - 1);
    }
    layout->addRow(Tr::tr("Python interpreter:"), interpreters);
    auto pathChooser = new PathChooser();
    pathChooser->setInitialBrowsePathBackup(startDirectory);
    pathChooser->setExpectedKind(PathChooser::Directory);
    pathChooser->setPromptDialogTitle(Tr::tr("New Python Virtual Environment Directory"));
    layout->addRow(Tr::tr("Virtual environment directory:"), pathChooser);
    auto buttons = new QDialogButtonBox(QDialogButtonBox::Cancel);
    auto createButton = buttons->addButton(Tr::tr("Create"), QDialogButtonBox::AcceptRole);
    createButton->setEnabled(false);
    connect(pathChooser,
            &PathChooser::validChanged,
            createButton,
            [createButton](bool valid) { createButton->setEnabled(valid); });
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addRow(buttons);
    dialog.setLayout(layout);
    if (dialog.exec() == QDialog::Rejected) {
        callback({});
        return;
    }

    const Interpreter interpreter = PythonSettings::interpreter(
        interpreters->currentData().toString());

    auto venvDir = pathChooser->filePath();
    createVirtualEnvironment(interpreter.command, venvDir, callback);
}

void PythonSettings::createVirtualEnvironment(
    const FilePath &python,
    const FilePath &directory,
    const std::function<void(const FilePath &)> &callback)
{
    QTC_ASSERT(python.isExecutableFile(), return);
    QTC_ASSERT(!directory.exists() || directory.isDir(), return);

    const CommandLine command(python, QStringList{"-m", "venv", directory.toUserOutput()});

    auto process = new Process;
    auto progress = new Core::ProcessProgress(process);
    progress->setDisplayName(Tr::tr("Create Python venv"));
    QObject::connect(process, &Process::done, [directory, process, callback](){
        if (process->result() == ProcessResult::FinishedWithSuccess) {
            FilePath venvPython = directory.osType() == Utils::OsTypeWindows ? directory / "Scripts"
                                                                             : directory / "bin";
            venvPython = venvPython.pathAppended("python").withExecutableSuffix();
            if (venvPython.exists()) {
                if (callback)
                    callback(venvPython);
                emit instance()->virtualEnvironmentCreated(venvPython);
            }
        }
        process->deleteLater();
    });
    process->setCommand(command);
    process->start();
}

QList<Interpreter> PythonSettings::detectPythonVenvs(const FilePath &path)
{
    QList<Interpreter> result;
    QDir dir = path.toFileInfo().isDir() ? QDir(path.toString()) : path.toFileInfo().dir();
    if (dir.exists()) {
        const QString venvPython = HostOsInfo::withExecutableSuffix("python");
        const QString activatePath = HostOsInfo::isWindowsHost() ? QString{"Scripts"}
                                                                 : QString{"bin"};
        do {
            for (const QString &directory : dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
                if (dir.cd(directory)) {
                    if (dir.cd(activatePath)) {
                        if (dir.exists("activate") && dir.exists(venvPython)) {
                            FilePath python = FilePath::fromString(dir.absoluteFilePath(venvPython));
                            dir.cdUp();
                            const QString defaultName = QString("Python (%1 Virtual Environment)")
                                                            .arg(dir.dirName());
                            Interpreter interpreter
                                = Utils::findOrDefault(PythonSettings::interpreters(),
                                                       Utils::equal(&Interpreter::command, python));
                            if (interpreter.command.isEmpty()) {
                                interpreter = createInterpreter(python, defaultName);
                                PythonSettings::addInterpreter(interpreter);
                            }
                            result << interpreter;
                        } else {
                            dir.cdUp();
                        }
                    }
                    dir.cdUp();
                }
            }
        } while (dir.cdUp() && !(dir.isRoot() && Utils::HostOsInfo::isAnyUnixHost()));
    }
    return result;
}

void PythonSettings::initFromSettings(QtcSettings *settings)
{
    settings->beginGroup(settingsGroupKey);
    const QVariantList interpreters = settings->value(interpreterKey).toList();
    QList<Interpreter> oldSettings;
    for (const QVariant &interpreterVar : interpreters) {
        auto interpreterList = interpreterVar.toList();
        const Interpreter interpreter{interpreterList.value(0).toString(),
                                      interpreterList.value(1).toString(),
                                      FilePath::fromSettings(interpreterList.value(2)),
                                      interpreterList.value(3, true).toBool()};
        if (interpreterList.size() == 3)
            oldSettings << interpreter;
        else if (interpreterList.size() == 4)
            m_interpreters << interpreter;
    }

    for (const Interpreter &interpreter : std::as_const(oldSettings)) {
        if (Utils::anyOf(m_interpreters, Utils::equal(&Interpreter::id, interpreter.id)))
            continue;
        m_interpreters << interpreter;
    }

    const auto keepInterpreter = [](const Interpreter &interpreter) {
        return !interpreter.autoDetected // always keep user added interpreters
                || !interpreter.command.isLocal() // remote devices might not be reachable at startup
                || interpreter.command.isExecutableFile();
    };

    const auto [valid, outdatedInterpreters] = Utils::partition(m_interpreters, keepInterpreter);
    m_interpreters = valid;

    if (!settings->value(kitsGeneratedKey, false).toBool()) {
        for (const Interpreter &interpreter : m_interpreters) {
            if (interpreter.autoDetected) {
                const FilePath &cmd = interpreter.command;
                if (!cmd.isLocal() || cmd.parentDir().pathAppended("activate").exists())
                    continue;
            }
            addKitsForInterpreter(interpreter, false);
        }
    } else {
        fixupPythonKits();
    }

    for (const Interpreter &outdated : outdatedInterpreters)
        removeKitsForInterpreter(outdated);

    m_defaultInterpreterId = settings->value(defaultKey).toString();

    QVariant pylsEnabled = settings->value(pylsEnabledKey);
    if (pylsEnabled.isNull())
        disableOutdatedPyls();
    else
        m_pylsEnabled = pylsEnabled.toBool();
    const QVariant pylsConfiguration = settings->value(pylsConfigurationKey);
    if (!pylsConfiguration.isNull())
        m_pylsConfiguration = pylsConfiguration.toString();
    else
        m_pylsConfiguration = defaultPylsConfiguration();
    settings->endGroup();
}

void PythonSettings::writeToSettings(QtcSettings *settings)
{
    settings->beginGroup(settingsGroupKey);
    QVariantList interpretersVar;
    for (const Interpreter &interpreter : m_interpreters) {
        QVariantList interpreterVar{interpreter.id,
                                    interpreter.name,
                                    interpreter.command.toSettings()};
        interpretersVar.append(QVariant(interpreterVar)); // old settings
        interpreterVar.append(interpreter.autoDetected);
        interpretersVar.append(QVariant(interpreterVar)); // new settings
    }
    settings->setValue(interpreterKey, interpretersVar);
    settings->setValue(defaultKey, m_defaultInterpreterId);

    settings->setValueWithDefault(pylsConfigurationKey,
                                  m_pylsConfiguration,
                                  defaultPylsConfiguration());

    settings->setValue(pylsEnabledKey, m_pylsEnabled);
    settings->setValue(kitsGeneratedKey, true);
    settings->endGroup();
}

void PythonSettings::detectPythonOnDevice(const Utils::FilePaths &searchPaths,
                                          const QString &deviceName,
                                          const QString &detectionSource,
                                          QString *logMessage)
{
    QStringList messages{Tr::tr("Searching Python binaries...")};
    auto alreadyConfigured = interpreterOptionsPage().interpreters();
    for (const FilePath &path : searchPaths) {
        const FilePath python = path.pathAppended("python3").withExecutableSuffix();
        if (!python.isExecutableFile())
            continue;
        if (Utils::contains(alreadyConfigured, Utils::equal(&Interpreter::command, python)))
            continue;
        auto interpreter = createInterpreter(python, "Python on", "on " + deviceName);
        interpreter.detectionSource = detectionSource;
        interpreterOptionsPage().addInterpreter(interpreter);
        messages.append(Tr::tr("Found \"%1\" (%2)").arg(interpreter.name, python.toUserOutput()));
    }
    if (logMessage)
        *logMessage = messages.join('\n');
}

void PythonSettings::removeDetectedPython(const QString &detectionSource, QString *logMessage)
{
    if (logMessage)
        logMessage->append(Tr::tr("Removing Python") + '\n');

    interpreterOptionsPage().removeInterpreterFrom(detectionSource);
}

void PythonSettings::listDetectedPython(const QString &detectionSource, QString *logMessage)
{
    if (!logMessage)
        return;
    logMessage->append(Tr::tr("Python:") + '\n');
    for (Interpreter &interpreter: interpreterOptionsPage().interpreterFrom(detectionSource))
        logMessage->append(interpreter.name + '\n');
}

void PythonSettings::fixupPythonKits()
{
    if (!KitManager::isLoaded()) {
        connect(KitManager::instance(),
                &KitManager::kitsLoaded,
                settingsInstance,
                &PythonSettings::fixupPythonKits,
                Qt::UniqueConnection);
        return;
    }
    for (const Interpreter &interpreter : m_interpreters) {
        if (auto k = KitManager::kit(Id::fromString(interpreter.id)))
            setRelevantAspectsToKit(k);
    }
}

void PythonSettings::saveSettings()
{
    QTC_ASSERT(settingsInstance, return);
    settingsInstance->writeToSettings(Core::ICore::settings());
    emit settingsInstance->interpretersChanged(settingsInstance->m_interpreters,
                                               settingsInstance->m_defaultInterpreterId);
}

QList<Interpreter> PythonSettings::interpreters()
{
    return settingsInstance->m_interpreters;
}

Interpreter PythonSettings::defaultInterpreter()
{
    return interpreter(settingsInstance->m_defaultInterpreterId);
}

Interpreter PythonSettings::interpreter(const QString &interpreterId)
{
    return Utils::findOrDefault(settingsInstance->m_interpreters,
                                Utils::equal(&Interpreter::id, interpreterId));
}

void setupPythonSettings(QObject *guard)
{
    new PythonSettings; // Initializes settingsInstance
    settingsInstance->setParent(guard);
}

Utils::ListModel<ProjectExplorer::Interpreter> *createInterpreterModel(QObject *parent)
{
    const auto model = new ListModel<Interpreter>(parent);
    model->setDataAccessor([](const Interpreter &interpreter, int column, int role) -> QVariant {
        if (interpreter.id == "none") {
            if (role == Qt::DisplayRole)
                return Tr::tr("None", "No Python interpreter");
            if (role == KitAspect::IsNoneRole)
                return true;
            return {};
        }
        switch (role) {
        case Qt::DisplayRole:
            return interpreter.name;
        case Qt::FontRole: {
            QFont f;
            f.setBold(interpreter.id == PythonSettings::defaultInterpreter().id);
            return f;
        }
        case Qt::ToolTipRole:
            if (!interpreter.command.isLocal())
                break;
            if (interpreter.command.isEmpty())
                return Tr::tr("Executable is empty.");
            if (!interpreter.command.exists())
                return Tr::tr("\"%1\" does not exist.").arg(interpreter.command.toUserOutput());
            if (!interpreter.command.isExecutableFile())
                return Tr::tr("\"%1\" is not an executable file.")
                    .arg(interpreter.command.toUserOutput());
            break;
        case Qt::DecorationRole:
            if (column == 0 && !PythonSettings::interpreterIsValid(interpreter))
                return Utils::Icons::CRITICAL.icon();
            break;
        case KitAspect::IdRole:
            return interpreter.id;
        case KitAspect::QualityRole:
            return int(PythonSettings::interpreterIsValid(interpreter));
        default:
            break;
        }
        return {};
    });
    model->setAllData(PythonSettings::interpreters());
    return model;
}

} // Python::Internal

#include "pythonsettings.moc"
