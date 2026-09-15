// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "pythonsettings.h"

#include "pythonconstants.h"
#include "pythonkitaspect.h"
#include "pythontr.h"
#include "pythonutils.h"

#include <coreplugin/dialogs/ioptionspage.h>
#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/progressmanager/processprogress.h>

#include <debugger/debuggerkitaspect.h>

#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/kitaspect.h>
#include <projectexplorer/environmentkitaspect.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <extensionsystem/pluginmanager.h>

#include <languageclient/languageclient_global.h>
#include <languageclient/languageclientsettings.h>
#include <languageclient/languageclientmanager.h>

#include <texteditor/textdocument.h>
#include <texteditor/texteditor.h>

#include <utils/algorithm.h>
#include <utils/async.h>
#include <utils/environment.h>
#include <utils/globaltasktree.h>
#include <utils/guiutils.h>
#include <utils/layoutbuilder.h>
#include <utils/listmodel.h>
#include <utils/pathchooser.h>
#include <utils/qtcprocess.h>
#include <utils/qtcassert.h>
#include <utils/treemodel.h>
#include <utils/utilsicons.h>
#include <utils/shutdownguard.h>

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QFormLayout>
#include <QGroupBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLoggingCategory>
#include <QPointer>
#include <QPushButton>
#include <QSettings>
#include <QStackedWidget>
#include <QTreeView>
#include <QVBoxLayout>
#include <QWidget>

#ifdef WITH_TESTS
#include <QFont>
#include <QScopeGuard>
#include <QTemporaryDir>
#include <QTest>
#endif // WITH_TESTS

using namespace ProjectExplorer;
using namespace QtTaskTree;
using namespace Utils;

namespace Python::Internal {

// This is defacto the unapplied state of PythonSettings::defaultInterpreter().id()
static QString s_defaultId;
// The default the settings had when s_defaultId last followed them.
static QString s_defaultIdFromSettings;

static Q_LOGGING_CATEGORY(pylspLog, "qtc.python.pylsp", QtWarningMsg)

InterpreterModel::InterpreterModel(const std::function<bool(QString)> &isDefaultId)
{
    setDataAccessor([isDefaultId](const Interpreter &interpreter, int column, int role) -> QVariant {
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
            f.setBold(isDefaultId && isDefaultId(interpreter.id));
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
    m_lastFromSettings = PythonSettings::interpreters();
    setAllData(m_lastFromSettings);
}

// This is defacto the unapplied state of PythonSettings::interpreters(), and has to follow
// whatever changes the settings behind the options page's back.
InterpreterModel &interpreterModel()
{
    static InterpreterModel theInterpreterModel([](const QString &id) { return id == s_defaultId; });
    [[maybe_unused]] static const bool defaultSynced = [] {
        s_defaultId = s_defaultIdFromSettings = PythonSettings::defaultInterpreterId();
        return true;
    }();
    [[maybe_unused]] static const QMetaObject::Connection connection = QObject::connect(
        PythonSettings::instance(),
        &PythonSettings::interpretersChanged,
        &theInterpreterModel,
        [](const QList<Interpreter> &interpreters, const QString &defaultId) {
            theInterpreterModel.mergeInterpreters(interpreters);
            // An unapplied "Make Default" outlives a write that leaves its interpreter alone.
            const bool madeDefault = s_defaultId != s_defaultIdFromSettings
                                     && Utils::contains(theInterpreterModel.interpreters(),
                                                        Utils::equal(&Interpreter::id, s_defaultId));
            const QString previousDefaultId = s_defaultId;
            if (!madeDefault)
                s_defaultId = defaultId;
            s_defaultIdFromSettings = defaultId;
            if (s_defaultId == previousDefaultId)
                return;
            // The bold font of the default row is not part of the row data merged above.
            for (const QString &id : {previousDefaultId, s_defaultId}) {
                const QModelIndex index = theInterpreterModel.findIndex(
                    Utils::equal(&Interpreter::id, id));
                if (index.isValid())
                    emit theInterpreterModel.dataChanged(index, index, {Qt::FontRole});
            }
        });
    return theInterpreterModel;
}

Interpreter PythonSettings::createInterpreter(
    const FilePath &python,
    const QString &defaultName,
    const QString &suffix,
    const DetectionSource &detectionSource)
{
    Interpreter result;
    result.id = QUuid::createUuid().toString();
    result.command = python;
    result.detectionSource = detectionSource;

    result.name = pythonVersion(python);
    if (result.name.isEmpty())
        result.name = defaultName;
    QDir pythonDir(python.parentDir().toUrlishString());
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
    InterpreterDetailsWidget()
    {
        m_executable.setExpectedKind(PathChooserKind::ExistingCommand);
        m_executable.setAllowPathFromDevice(true);

        connect(&m_name, &QLineEdit::textChanged, this, &InterpreterDetailsWidget::changed);
        connect(&m_executable, &PathChooser::textChanged, this, &InterpreterDetailsWidget::changed);

        using namespace Layouting;

        Form {
            Tr::tr("Name:"), &m_name, br,
            Tr::tr("Executable:"), &m_executable,
            noMargin
        }.attachTo(this);
    }

    void updateInterpreter(const Interpreter &interpreter)
    {
        QSignalBlocker blocker(this); // do not emit changed when we change the controls here
        m_currentInterpreter = interpreter;
        m_name.setText(interpreter.name);
        m_executable.setFilePath(interpreter.command);
    }

    Interpreter toInterpreter()
    {
        m_currentInterpreter.command = m_executable.filePath();
        m_currentInterpreter.name = m_name.text();
        return m_currentInterpreter;
    }
    QLineEdit m_name;
    PathChooser m_executable;
    Interpreter m_currentInterpreter;

signals:
    void changed();
};


class InterpreterOptionsWidget : public Core::IOptionsPageWidget
{
public:
    InterpreterOptionsWidget();

    void apply() override
    {
        PythonSettings::setInterpreter(interpreterModel().interpreters(), s_defaultId);
    }

    void cancel() override
    {
        interpreterModel().setInterpreters(PythonSettings::interpreters());
        s_defaultId = s_defaultIdFromSettings = PythonSettings::defaultInterpreterId();
    }

private:
    void currentChanged(const QModelIndex &index, const QModelIndex &previous);
    void detailsChanged();
    void updateCleanButton();
    void updateGenerateKitButton(const Interpreter &interpreter);
    void addItem();
    void deleteItem();
    void makeDefault();
    void generateKit();
    void cleanUp();

    QTreeView m_view;
    InterpreterDetailsWidget m_detailsWidget;
    QPushButton m_addButton;
    QPushButton m_deleteButton;
    QPushButton m_makeDefaultButton;
    QPushButton m_generateKitButton;
    QPushButton m_cleanButton;
};

InterpreterOptionsWidget::InterpreterOptionsWidget()
{
    m_addButton.setText(Tr::tr("&Add"));

    m_deleteButton.setText(Tr::tr("&Delete"));
    m_deleteButton.setEnabled(false);
    m_makeDefaultButton.setText(Tr::tr("&Make Default"));
    m_makeDefaultButton.setEnabled(false);
    m_generateKitButton.setText(Tr::tr("&Generate Kit"));
    m_generateKitButton.setEnabled(false);

    m_cleanButton.setText(Tr::tr("&Clean Up"));
    m_cleanButton.setToolTip(Tr::tr("Remove all Python interpreters without a valid executable."));

    using namespace Layouting;
    Row {
        Column {
            m_view,
            m_detailsWidget
        },
        Column {
            m_addButton,
            m_deleteButton,
            m_makeDefaultButton,
            m_generateKitButton,
            m_cleanButton,
            st
        }
    }.attachTo(this);

    updateCleanButton();

    m_detailsWidget.hide();

    m_view.setModel(&interpreterModel());
    m_view.setHeaderHidden(true);
    m_view.setSelectionMode(QAbstractItemView::SingleSelection);
    m_view.setSelectionBehavior(QAbstractItemView::SelectItems);

    connect(&m_addButton, &QPushButton::pressed, this, &InterpreterOptionsWidget::addItem);
    connect(&m_deleteButton, &QPushButton::pressed, this, &InterpreterOptionsWidget::deleteItem);
    connect(&m_makeDefaultButton, &QPushButton::pressed, this, &InterpreterOptionsWidget::makeDefault);
    connect(&m_generateKitButton, &QPushButton::pressed, this, &InterpreterOptionsWidget::generateKit);
    connect(&m_cleanButton, &QPushButton::pressed, this, &InterpreterOptionsWidget::cleanUp);

    connect(&m_detailsWidget, &InterpreterDetailsWidget::changed,
            this, &InterpreterOptionsWidget::detailsChanged);
    connect(m_view.selectionModel(), &QItemSelectionModel::currentChanged,
            this, &InterpreterOptionsWidget::currentChanged);
    // A row the settings changed from outside must not be written back by currentChanged().
    connect(&interpreterModel(), &QAbstractItemModel::dataChanged, this,
            [this](const QModelIndex &topLeft, const QModelIndex &bottomRight) {
        const QModelIndex current = m_view.currentIndex();
        if (!current.isValid() || current.row() < topLeft.row() || current.row() > bottomRight.row())
            return;
        const Interpreter interpreter = interpreterModel().itemAt(current.row())->itemData;
        if (interpreter != m_detailsWidget.toInterpreter())
            m_detailsWidget.updateInterpreter(interpreter);
    });

    installMarkSettingsDirtyTriggerRecursively(this);
}

void InterpreterModel::setInterpreters(const QList<Interpreter> &interpreters)
{
    m_lastFromSettings = interpreters;
    clear();
    for (const Interpreter &interpreter : interpreters)
        appendItem(interpreter);
}

// Only what actually changed in the settings is carried over, so that edits the options page
// has not applied yet survive a write from elsewhere: a row the settings lost goes, a row they
// gained comes, a row they changed follows unless the user edited it too, and everything else
// stays as the user left it.
void InterpreterModel::mergeInterpreters(const QList<Interpreter> &interpreters)
{
    const QList<Interpreter> before = m_lastFromSettings;
    m_lastFromSettings = interpreters;

    const auto hasId = [](const QList<Interpreter> &list, const QString &id) {
        return Utils::contains(list, Utils::equal(&Interpreter::id, id));
    };

    destroyItems([&](const Interpreter &interpreter) {
        return hasId(before, interpreter.id) && !hasId(interpreters, interpreter.id);
    });

    for (const Interpreter &interpreter : interpreters) {
        const auto sameId = Utils::equal(&Interpreter::id, interpreter.id);
        if (ChildType *item = findItemByData(sameId)) {
            if (item->itemData != interpreter
                && item->itemData == Utils::findOrDefault(before, sameId)) {
                item->itemData = interpreter;
                item->update();
            }
        } else if (!hasId(before, interpreter.id)) {
            appendItem(interpreter);
        }
    }
}

QList<Interpreter> InterpreterModel::interpreters() const
{
    QList<Interpreter> interpreters;
    for (const TreeItem *treeItem : *this)
        interpreters << static_cast<const ListItem<Interpreter> *>(treeItem)->itemData;
    return interpreters;
}

void InterpreterOptionsWidget::currentChanged(const QModelIndex &index, const QModelIndex &previous)
{
    if (previous.isValid()) {
        interpreterModel().itemAt(previous.row())->itemData = m_detailsWidget.toInterpreter();
        emit interpreterModel().dataChanged(previous, previous);
    }
    if (index.isValid()) {
        const Interpreter interpreter = interpreterModel().itemAt(index.row())->itemData;
        m_detailsWidget.updateInterpreter(interpreter);
        m_detailsWidget.show();
        updateGenerateKitButton(interpreter);
    } else {
        m_detailsWidget.hide();
        m_generateKitButton.setEnabled(false);
    }
    m_deleteButton.setEnabled(index.isValid());
    m_makeDefaultButton.setEnabled(index.isValid());
}

void InterpreterOptionsWidget::detailsChanged()
{
    const QModelIndex &index = m_view.currentIndex();
    if (index.isValid()) {
        const Interpreter interpreter = m_detailsWidget.toInterpreter();
        interpreterModel().itemAt(index.row())->itemData = interpreter;
        emit interpreterModel().dataChanged(index, index);
        updateGenerateKitButton(interpreter);
    }
    updateCleanButton();
}

void InterpreterOptionsWidget::updateCleanButton()
{
    m_cleanButton.setEnabled(Utils::anyOf(interpreterModel().allData(), [](const Interpreter &interpreter) {
        return !interpreter.command.isExecutableFile();
    }));
}

void InterpreterOptionsWidget::updateGenerateKitButton(const Interpreter &interpreter)
{
    bool enabled = !KitManager::kit(Id::fromString(interpreter.id))
            && (!interpreter.command.isLocal() || interpreter.command.isExecutableFile());
    m_generateKitButton.setEnabled(enabled);
}

void InterpreterOptionsWidget::addItem()
{
    const QModelIndex &index = interpreterModel().indexForItem(interpreterModel().appendItem(
        {QUuid::createUuid().toString(), QString("Python"), FilePath(), DetectionSource::Manual}));
    QTC_ASSERT(index.isValid(), return);
    m_view.setCurrentIndex(index);
    updateCleanButton();
    markSettingsDirty();
}

void InterpreterOptionsWidget::deleteItem()
{
    const QModelIndex &index = m_view.currentIndex();
    if (index.isValid()) {
        interpreterModel().destroyItem(interpreterModel().itemAt(index.row()));
        markSettingsDirty();
    }
    updateCleanButton();
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
        setIgnoreForDirtyHook(advanced);
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

        installMarkSettingsDirtyTriggerRecursively(this);
        connect(m_editor->textDocument(), &TextEditor::TextDocument::contentsChangedWithPosition,
                this, markSettingsDirty);
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
        setWidgetCreator([] {return new PyLSConfigureWidget();});
    }
};

static PyLSOptionsPage &pylspOptionsPage()
{
    static PyLSOptionsPage page;
    return page;
}

void InterpreterOptionsWidget::makeDefault()
{
    const QModelIndex &index = m_view.currentIndex();
    if (index.isValid()) {
        QModelIndex defaultIndex = interpreterModel().findIndex([](const Interpreter &interpreter) {
            return interpreter.id == s_defaultId;
        });
        s_defaultId = interpreterModel().itemAt(index.row())->itemData.id;
        emit interpreterModel().dataChanged(index, index, {Qt::FontRole});
        if (defaultIndex.isValid())
            emit interpreterModel().dataChanged(defaultIndex, defaultIndex, {Qt::FontRole});
        if (!defaultIndex.isValid() || defaultIndex != index)
            markSettingsDirty();
    }
}

void InterpreterOptionsWidget::generateKit()
{
    const QModelIndex &index = m_view.currentIndex();
    if (index.isValid())
        PythonSettings::addKitsForInterpreter(interpreterModel().itemAt(index.row())->itemData, true);
    m_generateKitButton.setEnabled(false);
}

void InterpreterOptionsWidget::cleanUp()
{
    interpreterModel().destroyItems(
        [](const Interpreter &interpreter) { return !interpreter.command.isExecutableFile(); });
    updateCleanButton();
    markSettingsDirty();
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
        if (setting->settingsTypeId() != LanguageClient::Constants::LANGUAGECLIENT_STDIO_SETTINGS_ID)
            continue;
        auto stdioSetting = static_cast<const StdIOSettings *>(setting);
        if (stdioSetting->arguments().startsWith("-m pyls")
                && stdioSetting->languageFilter().isSupported("foo.py", Constants::C_PY_MIMETYPE)) {
            LanguageClientManager::enableClientSettings(stdioSetting->id(), false);
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
                pythons << PythonSettings::createInterpreter(python, "Python " + versionGroup);
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
                pythons << PythonSettings::createInterpreter(executable, "Python from Path");
        }
    } else {
        const QStringList filters = {"python",
                                     "python[1-9].[0-9]",
                                     "python[1-9].[1-9][0-9]",
                                     "python[1-9]"};
        const FilePaths dirs = Environment::systemEnvironment().path();
        QSet<FilePath> used;
        for (const FilePath &path : dirs) {
            const QDir dir(path.toUrlishString());
            for (const QFileInfo &fi : dir.entryInfoList(filters)) {
                if (promise.isCanceled())
                    return;

                const FilePath executable = FilePath::fromUserInput(fi.canonicalFilePath());
                if (!used.contains(executable) && executable.exists()) {
                    used.insert(executable);
                    pythons << PythonSettings::createInterpreter(executable, "Python from Path");
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
    // Not findOrDefault(): a default-constructed Interpreter comes with a fresh id, which
    // would end up as an id no interpreter has.
    for (const Interpreter &interpreter : pythons) {
        if (interpreter.command == pythonFromPath)
            return interpreter.id;
    }
    return {};
}

static PythonSettings *settingsInstance = nullptr;

static std::optional<Interpreter> registeredInterpreter(
    const QList<Interpreter> &interpreters, const FilePath &candidate)
{
    const FilePath resolvedCandidate = candidate.resolveSymlinks();
    for (const Interpreter &interpreter : interpreters) {
        if (interpreter.command.isSameDevice(candidate)
            && interpreter.command.resolveSymlinks() == resolvedCandidate) {
            return interpreter;
        }
    }
    return std::nullopt;
}

static bool alreadyRegistered(const QList<Interpreter> &interpreters, const FilePath &candidate)
{
    return registeredInterpreter(interpreters, candidate).has_value();
}

static bool alreadyRegistered(const Interpreter &candidate)
{
    return alreadyRegistered(settingsInstance->interpreters(), candidate.command);
}

std::optional<QStringList> activePythonVersions(const QList<Interpreter> &interpreters)
{
    QStringList versions;
    for (const Interpreter &interpreter : interpreters) {
        if (!interpreter.command.isLocal() || !interpreter.command.isExecutableFile())
            continue;
        const QString version = pythonVersion(interpreter.command);
        // The interpreter is there, we just cannot read its version right now.
        if (version.isEmpty()) {
            qCDebug(pylspLog) << "Not pruning, no version for" << interpreter.command;
            return std::nullopt;
        }
        versions << version;
    }
    return versions;
}

void prunePylspInstallations(const FilePath &pylspRoot, const QStringList &keepVersions)
{
    if (!pylspRoot.isDir())
        return;

    const FilePaths versionDirs
        = pylspRoot.dirEntries(DirFilterFlag::Dirs | DirFilterFlag::NoDotAndDotDot);
    for (const FilePath &dir : versionDirs) {
        if (keepVersions.contains(dir.fileName()))
            continue;
        qCDebug(pylspLog) << "Removing stale pylsp installation" << dir;
        const Result<> removed = dir.removeRecursively();
        QTC_CHECK_RESULT(removed);
    }
}

static void pruneStalePylspInstallations(const QList<Interpreter> &interpreters)
{
    if (const std::optional<QStringList> versions = activePythonVersions(interpreters))
        prunePylspInstallations(localPylspRoot(), *versions);
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
    const auto onPruneSetup = [](Async<void> &task) {
        task.setConcurrentCallData(pruneStalePylspInstallations, settingsInstance->m_interpreters);
    };

    // The group is sequential, so the prune sees the settled interpreter list.
    const Group recipe {
        finishAllAndSuccess,
        Utils::HostOsInfo::isWindowsHost()
            ? AsyncTask<QList<Interpreter>>(onRegistrySetup, onTaskDone) : nullItem,
        AsyncTask<QList<Interpreter>>(onPathSetup, onTaskDone),
        AsyncTask<void>(onPruneSetup)
    };
    m_taskTreeRunner.start(recipe);

    if (m_defaultInterpreterId.isEmpty())
        m_defaultInterpreterId = idForPythonFromPath(m_interpreters);

    s_defaultId = s_defaultIdFromSettings = m_defaultInterpreterId;

    writeToSettings(Core::ICore::settings());

    connect(DeviceManager::instance(), &DeviceManager::toolDetectionRequested,
            this, [](Id devId, const FilePaths &searchPaths, quint64 token,
                     const ToolDetectionLogger &logger) {
        const IDevicePtr device = DeviceManager::find(devId);
        QTC_ASSERT(device, return);
        device->registerToolDetectionTask(token);
        if (logger)
            logger.logTopLevel(Tr::tr("Searching for Python interpreters..."));

        const bool isDesktopDevice = devId == ProjectExplorer::Constants::DESKTOP_DEVICE_ID;
        const DetectionSource detectionSource(
            DetectionSource::FromSystem, isDesktopDevice ? QString() : devId.toString());
        const LogCallback logCallback = [logger](const QString &message) {
            if (logger)
                logger.logItem(message);
        };
        const QList<Interpreter> known = PythonSettings::interpreters();
        const ExecutableItem detector
            = PythonSettings::autoDetect(nullptr, searchPaths, detectionSource, logCallback);
        GlobalTaskTree::start(Group{detector}, {}, [devId, token, known, logger] {
            if (logger && PythonSettings::interpreters() == known)
                logger.logItem(Tr::tr("No new Python interpreters found."));
            if (const IDevicePtr device = DeviceManager::find(devId))
                device->deregisterToolDetectionTask(token);
        });
    });

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
    relevantAspects.unite(
        {PythonKitAspect::id(), EnvironmentKitAspect::id(), Debugger::DebuggerKitAspect::id()});
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
                if (interpreter.detectionSource.id.isEmpty())
                    k->setDetectionSource({DetectionSource::FromSystem, "Python"});
                else
                    k->setDetectionSource(interpreter.detectionSource);

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
    const bool defaultIsThere
        = Utils::anyOf(interpreters, Utils::equal(&Interpreter::id, defaultId));
    const QString newDefaultId = defaultIsThere ? defaultId : idForPythonFromPath(interpreters);
    if (newDefaultId == settingsInstance->m_defaultInterpreterId
        && interpreters == settingsInstance->m_interpreters) {
        return;
    }
    QList<Interpreter> toRemove = settingsInstance->m_interpreters;
    for (const Interpreter &interpreter : interpreters) {
        if (!Utils::eraseOne(toRemove, Utils::equal(&Interpreter::id, interpreter.id)))
            addKitsForInterpreter(interpreter, false);
    }
    for (const Interpreter &interpreter : std::as_const(toRemove))
        removeKitsForInterpreter(interpreter);
    settingsInstance->m_interpreters = interpreters;
    settingsInstance->m_defaultInterpreterId = newDefaultId;
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
    markSettingsDirty();
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
    pathChooser->setExpectedKind(PathChooserKind::Directory);
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

static QSet<FilePath> &venvsCurrentlySettingUp()
{
    static QSet<FilePath> venvs;
    return venvs;
}

bool PythonSettings::isRunningVirtualEnvironmentSetup(const FilePath &python)
{
    return Utils::anyOf(venvsCurrentlySettingUp(), [python](const FilePath &venv) {
        return python.isChildOf(venv);
    });
}

void PythonSettings::createVirtualEnvironment(
    const FilePath &python,
    const FilePath &directory,
    const std::function<void(const FilePath &)> &callback)
{
    QTC_ASSERT(python.isExecutableFile(), return);
    QTC_ASSERT(!directory.exists() || directory.isDir(), return);

    const CommandLine command(python, QStringList{"-m", "venv", directory.nativePath()});
    venvsCurrentlySettingUp().insert(directory);

    auto process = new Process;
    auto progress = new Core::ProcessProgress(process);
    progress->setDisplayName(Tr::tr("Create Python venv"));
    QObject::connect(process, &Process::done, [directory, process, callback](){
        venvsCurrentlySettingUp().remove(directory);
        switch (process->result()) {
        case ProcessResult::FinishedWithSuccess: {
            FilePath venvPython = directory.osType() == Utils::OsTypeWindows ? directory / "Scripts"
                                                                             : directory / "bin";
            venvPython = venvPython.pathAppended("python").withExecutableSuffix();
            if (venvPython.exists()) {
                if (callback)
                    callback(venvPython);
                emit instance()->virtualEnvironmentCreated(venvPython);
            }
            break;
        }
        case ProcessResult::FinishedWithError:
            Core::MessageManager::writeFlashing(
                Tr::tr("Venv creation failed:\n%1").arg(process->allOutput()));
            break;
        case ProcessResult::TerminatedAbnormally:
            Core::MessageManager::writeFlashing(
                Tr::tr("Venv creation terminated abnormally:\n%1").arg(process->errorString()));
            break;
        case ProcessResult::StartFailed:
            Core::MessageManager::writeFlashing(
                Tr::tr("Venv creation could not be started:\n%1").arg(process->allOutput()));
            break;
        case ProcessResult::Canceled:
            Core::MessageManager::writeFlashing(Tr::tr("Venv creation canceled."));
            break;
        }

        process->deleteLater();
    });
    process->setCommand(command);
    process->start();
}

QList<Interpreter> PythonSettings::detectPythonVenvs(const FilePath &path)
{
    QList<Interpreter> result;
    QDir dir = path.toFileInfo().isDir() ? QDir(path.toUrlishString()) : path.toFileInfo().dir();
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
    const QVariantList interpreterList = settings->value(interpreterKey).toList();
    for (const QVariant &interpreterVariant : interpreterList) {
        if (isStore(interpreterVariant)) {
            Interpreter interpreter;
            interpreter.fromMap(Utils::storeFromVariant(interpreterVariant));
            Utils::erase(m_interpreters, Utils::equal(&Interpreter::id, interpreter.id));
            m_interpreters << interpreter;
            continue;
        }

        auto interpreterMembers = interpreterVariant.toList();
        if (interpreterMembers.size() <= 3)
            continue; // old settings, skip

        const auto id = interpreterMembers.value(0).toString();
        if (Utils::contains(m_interpreters, Utils::equal(&Interpreter::id, id)))
            continue; // already exists

        const bool isAutoDetected = interpreterMembers.value(3, true).toBool();
        const QString detectionSourceId = interpreterMembers.value(4, QString()).toString();
        DetectionSource detectionSource{
            isAutoDetected ? DetectionSource::FromSystem : DetectionSource::Manual,
            detectionSourceId};

        m_interpreters << Interpreter{
            id,
            interpreterMembers.value(1).toString(),
            FilePath::fromSettings(interpreterMembers.value(2)),
            detectionSource};
    }

    const auto keepInterpreter = [](const Interpreter &interpreter) {
        return !interpreter.detectionSource.isAutoDetected() // always keep user added interpreters
               || !interpreter.command.isLocal() // remote devices might not be reachable at startup
               || interpreter.command.isExecutableFile();
    };

    const auto [valid, outdatedInterpreters] = Utils::partition(m_interpreters, keepInterpreter);
    m_interpreters = valid;

    const bool kitsGenerated = settings->value(kitsGeneratedKey, false).toBool();
    if (kitsGenerated)
        fixupPythonKits();
    for (const Interpreter &interpreter : std::as_const(m_interpreters)) {
        if (!kitsGenerated) {
            if (interpreter.detectionSource.isAutoDetected()) {
                const FilePath &cmd = interpreter.command;
                if (!cmd.isLocal() || cmd.parentDir().pathAppended("activate").exists())
                    continue;
            }
            addKitsForInterpreter(interpreter, false);
        }
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
    QVariantList interpretersList;
    for (const Interpreter &interpreter : std::as_const(m_interpreters)) {
        if (interpreter.detectionSource.isTemporary())
            continue;

        QVariantList members{
            interpreter.id,
            interpreter.name,
            interpreter.command.toSettings(),
            interpreter.detectionSource.isAutoDetected()};
        // We need to cast to QVariant() here, otherwise interpretersList will simply append each
        // member as a separate item.
        interpretersList.append(QVariant(members)); // old settings
        Store newSettings;
        interpreter.toMap(newSettings);
        interpretersList.append(variantFromStore(newSettings)); // new settings
    }
    settings->setValue(interpreterKey, interpretersList);
    settings->setValue(defaultKey, m_defaultInterpreterId);

    settings->setValueWithDefault(pylsConfigurationKey,
                                  m_pylsConfiguration,
                                  defaultPylsConfiguration());

    settings->setValue(pylsEnabledKey, m_pylsEnabled);
    settings->setValue(kitsGeneratedKey, true);
    settings->endGroup();
}

QString PythonSettings::defaultInterpreterId()
{
    return settingsInstance->m_defaultInterpreterId;
}

ExecutableItem PythonSettings::autoDetect(
    Kit *kit,
    const Utils::FilePaths &searchPaths,
    const DetectionSource &detectionSource,
    const LogCallback &logCallback)
{
    const auto setupSearch = [searchPaths, detectionSource](Async<Interpreter> &task) {
        const QList<Interpreter> alreadyConfigured = PythonSettings::interpreters();

        task.setConcurrentCallData(
            [](QPromise<Interpreter> &promise,
               const FilePaths &searchPaths,
               const QList<Interpreter> &alreadyConfigured,
               const DetectionSource &detectionSource) {
                FilePaths reported;
                for (const FilePath &path : searchPaths) {
                    const FilePath python = path.pathAppended("python3").withExecutableSuffix();
                    if (!python.isExecutableFile())
                        continue;
                    // Several search paths can lead to the same binary, as /bin and /usr/bin do.
                    const FilePath resolved = python.resolveSymlinks();
                    if (reported.contains(resolved))
                        continue;
                    reported.append(resolved);
                    // Reported as well, a kit detection wants what an earlier run registered.
                    if (const std::optional<Interpreter> registered
                        = registeredInterpreter(alreadyConfigured, python)) {
                        promise.addResult(*registered);
                        continue;
                    }

                    Interpreter interpreter = PythonSettings::createInterpreter(
                        python, {}, "(" + python.toUserOutput() + ")", detectionSource);

                    promise.addResult(interpreter);
                }
            },
            searchPaths,
            alreadyConfigured,
            detectionSource);
    };

    const auto searchDone = [kitId = kit ? kit->id() : Id(), logCallback](
                                const Async<Interpreter> &task) {
        const QList<Interpreter> results = task.results();
        const QList<Interpreter> known = PythonSettings::interpreters();
        QList<Interpreter> registered;
        QList<Interpreter> found;
        for (const Interpreter &result : results) {
            // A detection that ran meanwhile may have registered the same command under an id
            // of its own. The command is compared as is, resolving links here could block.
            const auto match = std::find_if(
                known.cbegin(), known.cend(), [&result](const Interpreter &i) {
                    return i.id == result.id || i.command == result.command;
                });
            if (match == known.cend())
                found.append(result);
            else
                registered.append(*match);
        }
        for (const Interpreter &interpreter : found) {
            // The kit the search runs for gets the interpreter, it needs no kit of its own.
            if (kitId.isValid())
                settingsInstance->m_interpreters.append(interpreter);
            else
                addInterpreter(interpreter);
            logCallback(
                Tr::tr("Found \"%1\" (%2).")
                    .arg(interpreter.name, interpreter.command.toUserOutput()));
        }
        if (!kitId.isValid())
            return;
        if (!found.isEmpty())
            saveSettings();
        // An earlier run over the same paths registered these already, the kit still wants one.
        const QList<Interpreter> candidates = found.isEmpty() ? registered : found;
        if (candidates.isEmpty())
            return;
        // Kit is not a QObject, so the kit the search was started for may be gone by now.
        if (Kit *kit = KitManager::kit(kitId); kit && !PythonKitAspect::python(kit))
            PythonKitAspect::setPython(kit, candidates.first().id);
    };

    return AsyncTask<Interpreter>(setupSearch, searchDone);
}

void PythonSettings::removeDetectedPython(
    const QString &detectionSource, const LogCallback &logCallback)
{
    QList<Interpreter> remaining;
    for (const Interpreter &interpreter : std::as_const(settingsInstance->m_interpreters)) {
        if (interpreter.detectionSource.id == detectionSource)
            logCallback(Tr::tr("Removing Python: %1.").arg(interpreter.name));
        else
            remaining.append(interpreter);
    }
    const QString defaultId = settingsInstance->m_defaultInterpreterId;
    setInterpreter(remaining, defaultId);
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
    for (const Interpreter &interpreter : std::as_const(m_interpreters)) {
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

void setupPythonSettings()
{
    static GuardedObject thePythonSettings{new PythonSettings};
}

// InterpreterOptionsPage

class InterpreterOptionsPage : public Core::IOptionsPage
{
public:
    InterpreterOptionsPage()
    {
        setId(Constants::C_PYTHONOPTIONS_PAGE_ID);
        setDisplayName(Tr::tr("Interpreters"));
        setCategory(Constants::C_PYTHON_SETTINGS_CATEGORY);
        setWidgetCreator([] { return new InterpreterOptionsWidget; });
        setFixedKeywords({
            Tr::tr("Name:"),
            Tr::tr("Executable:"),
            Tr::tr("&Add"),
            Tr::tr("&Delete"),
            Tr::tr("&Clean Up"),
            Tr::tr("&Make Default"),
            Tr::tr("&Generate Kit"),
            PathChooser::browseButtonLabel()
        });
    }
};

static InterpreterOptionsPage page;

#ifdef WITH_TESTS

class InterpreterOptionsTest final : public QObject
{
    Q_OBJECT

private slots:
    void testKeepsAnUnappliedMakeDefault();
    void testShowsARowChangedElsewhere();
};

static QModelIndex indexOf(const QString &id)
{
    return interpreterModel().findIndex(Utils::equal(&Interpreter::id, id));
}

static bool isShownAsDefault(const QString &id)
{
    return interpreterModel().data(indexOf(id), Qt::FontRole).value<QFont>().bold();
}

void InterpreterOptionsTest::testKeepsAnUnappliedMakeDefault()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const FilePath dir = FilePath::fromUserInput(tmp.path());

    const QList<Interpreter> before = PythonSettings::interpreters();
    const QString defaultId = PythonSettings::defaultInterpreterId();
    const QScopeGuard restoreInterpreters(
        [before, defaultId] { PythonSettings::setInterpreter(before, defaultId); });

    const Interpreter first = PythonSettings::createInterpreter(dir / "python3", "First");
    PythonSettings::addInterpreter(first);
    const Interpreter second = PythonSettings::createInterpreter(dir / "python3.14", "Second");
    PythonSettings::addInterpreter(second, true);

    InterpreterOptionsWidget widget;
    auto view = widget.findChild<QTreeView *>();
    QVERIFY(view);
    QPushButton *makeDefault = Utils::findOrDefault(
        widget.findChildren<QPushButton *>(),
        [](QPushButton *button) { return button->text() == Tr::tr("&Make Default"); });
    QVERIFY(makeDefault);

    view->setCurrentIndex(indexOf(first.id));
    makeDefault->click();
    QVERIFY(isShownAsDefault(first.id));

    // A write that leaves the interpreter alone must not take the choice away.
    PythonSettings::addInterpreter(PythonSettings::createInterpreter(dir / "other", "Other"));
    QVERIFY(isShownAsDefault(first.id));
    QVERIFY(!isShownAsDefault(second.id));
}

void InterpreterOptionsTest::testShowsARowChangedElsewhere()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());

    const QList<Interpreter> before = PythonSettings::interpreters();
    const QString defaultId = PythonSettings::defaultInterpreterId();
    const QScopeGuard restoreInterpreters(
        [before, defaultId] { PythonSettings::setInterpreter(before, defaultId); });

    const Interpreter shown = PythonSettings::createInterpreter(
        FilePath::fromUserInput(tmp.path()) / "python3", "Shown");
    PythonSettings::addInterpreter(shown);

    InterpreterOptionsWidget widget;
    auto view = widget.findChild<QTreeView *>();
    QVERIFY(view);
    auto details = widget.findChild<InterpreterDetailsWidget *>();
    QVERIFY(details);

    view->setCurrentIndex(indexOf(shown.id));
    QCOMPARE(details->m_name.text(), shown.name);

    QList<Interpreter> renamed = PythonSettings::interpreters();
    const auto it = std::find_if(
        renamed.begin(), renamed.end(), Utils::equal(&Interpreter::id, shown.id));
    QVERIFY(it != renamed.end());
    it->name = "Renamed";
    PythonSettings::setInterpreter(renamed, PythonSettings::defaultInterpreterId());
    QCOMPARE(details->m_name.text(), QString("Renamed"));

    // Leaving the row writes the details back into it.
    view->setCurrentIndex({});
    const int row = indexOf(shown.id).row();
    QCOMPARE(interpreterModel().itemAt(row)->itemData.name, QString("Renamed"));
}

QObject *createInterpreterOptionsTest()
{
    return new InterpreterOptionsTest;
}

#endif // WITH_TESTS

} // Python::Internal

#include "pythonsettings.moc"
