// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "aiassistance.h"

#include "algorithm.h"
#include "guiutils.h"
#include "qtcassert.h"
#include "qtcsettings.h"
#include "shutdownguard.h"
#include "store.h"
#include "stylehelper.h"
#include "utilsicons.h"
#include "utilstr.h"

#include <QAction>
#include <QClipboard>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGuiApplication>
#include <QLabel>
#include <QListWidget>
#include <QMimeData>
#include <QPointer>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>

#include <utility>

/*!
    \namespace Utils::AiAssistance
    \inmodule QtCreator

    \brief The AiAssistance namespace connects code that wants an AI to do
    something with the plugins that can do it.

    A requester describes the work in an AiRequest: a prompt, context items
    such as files, text snippets and images, and whether the user is meant to
    continue the conversation (AiRequest::Interactive) or the requester wants
    the answer back (AiRequest::Headless). It passes the request to submit(),
    which lets the user pick a target, or to the overload taking an
    AiTargetSelection to bypass the picker.

    An AI integration derives from AiProvider, announces what it can talk to
    with setTargets(), and carries out requests in AiProvider::execute().

    A built-in provider copies the request to the clipboard as plain text, so
    a user can always paste it into an agent Qt Creator does not know about.
*/

namespace Utils {

const char settingsGroup[] = "AiAssistance";
const char lastSelectionKey[] = "LastSelection";
const char providerKey[] = "Provider";
const char targetKey[] = "Target";
const char parametersKey[] = "Parameters";
const char clipboardTargetId[] = "Clipboard";

// AiContextItem

/*!
    \class Utils::AiContextItem
    \inmodule QtCreator

    \brief The AiContextItem class is a piece of context that goes along with
    the prompt of an AiRequest.

    Create items with fromFile(), fromText(), and fromImage().
*/

/*!
    \enum Utils::AiContextItem::Kind

    This enum describes what an item holds.

    \value File  A file, possibly only some of its lines.
    \value Text  A text with a name.
    \value Image An image with a name.
*/

/*!
    \variable Utils::AiContextItem::kind
    \brief What the item holds.
*/

/*!
    \variable Utils::AiContextItem::name
    \brief The name that the item is shown with.
*/

/*!
    \variable Utils::AiContextItem::filePath
    \brief The file of a \l File item.
*/

/*!
    \variable Utils::AiContextItem::range
    \brief The lines of the file that the item is about, if not the whole file.
*/

/*!
    \variable Utils::AiContextItem::text
    \brief The text of a \l Text item.
*/

/*!
    \variable Utils::AiContextItem::image
    \brief The image of an \l Image item.
*/

/*!
    \class Utils::AiRequest
    \inmodule QtCreator

    \brief The AiRequest class describes a task for an AI.

    \sa AiAssistance::submit(), AiRequestTask
*/

/*!
    \enum Utils::AiRequest::Mode

    This enum describes whether the requester wants the answer back.

    \value Interactive The target takes the request over, and the user
           continues there, such as in a chat.
    \value Headless    The answer is reported back through the AiResponse.
*/

/*!
    \variable Utils::AiRequest::id
    \brief Identifies the kind of request.

    The target that the user picks is remembered for requests with the same id.
*/

/*!
    \variable Utils::AiRequest::title
    \brief The title of the dialog that lets the user pick a target.
*/

/*!
    \variable Utils::AiRequest::prompt
    \brief What the AI should do.
*/

/*!
    \variable Utils::AiRequest::context
    \brief The context that goes along with the prompt.
*/

/*!
    \variable Utils::AiRequest::workingDirectory
    \brief The directory that the AI works in.

    If it is empty, the provider picks one, such as the directory of the
    startup project.
*/

/*!
    \variable Utils::AiRequest::mode
    \brief Whether the answer is reported back.
*/

/*!
    \class Utils::AiParameterChoice
    \inmodule QtCreator

    \brief The AiParameterChoice class is one of the values that the user can
    choose for an AiTargetParameter.
*/

/*!
    \variable Utils::AiParameterChoice::value
    \brief The value that the provider gets.
*/

/*!
    \variable Utils::AiParameterChoice::displayName
    \brief The name that the user sees.
*/

/*!
    \variable Utils::AiParameterChoice::description
    \brief Describes the choice in more detail, for a tooltip.
*/

/*!
    \class Utils::AiTargetParameter
    \inmodule QtCreator

    \brief The AiTargetParameter class is an option of an AiTarget that the
    user sets for a request, such as the model.
*/

/*!
    \enum Utils::AiTargetParameter::Category

    This enum describes what a parameter is about.

    \value Model  The model that runs the request.
    \value Effort How much effort, such as thinking, goes into the request.
    \value Mode   The mode that the target runs the request in.
    \value Other  Anything else.
*/

/*!
    \variable Utils::AiTargetParameter::id
    \brief Identifies the parameter in an AiTargetSelection.
*/

/*!
    \variable Utils::AiTargetParameter::displayName
    \brief The name that the user sees.
*/

/*!
    \variable Utils::AiTargetParameter::category
    \brief What the parameter is about.
*/

/*!
    \variable Utils::AiTargetParameter::choices
    \brief The values that the user can choose.
*/

/*!
    \variable Utils::AiTargetParameter::defaultValue
    \brief The value of the choice that is used unless the user picks another.
*/

/*!
    \class Utils::AiTarget
    \inmodule QtCreator

    \brief The AiTarget class is something that an AiProvider can run requests
    on, such as an agent.
*/

/*!
    \variable Utils::AiTarget::id
    \brief Identifies the target within its provider.
*/

/*!
    \variable Utils::AiTarget::displayName
    \brief The name that the user sees.
*/

/*!
    \variable Utils::AiTarget::description
    \brief Describes the target in more detail, for a tooltip.
*/

/*!
    \variable Utils::AiTarget::icon
    \brief The icon that the user sees.
*/

/*!
    \variable Utils::AiTarget::supportsHeadless
    \brief Whether the target can report an answer back.

    Only such targets are offered for AiRequest::Headless requests.
*/

/*!
    \variable Utils::AiTarget::parameters
    \brief The options that the user sets for a request.
*/

/*!
    \class Utils::AiTargetSelection
    \inmodule QtCreator

    \brief The AiTargetSelection class names a target and the values of its
    parameters.
*/

/*!
    \variable Utils::AiTargetSelection::providerId
    \brief The provider of the target.
*/

/*!
    \variable Utils::AiTargetSelection::targetId
    \brief The target within the provider.
*/

/*!
    \variable Utils::AiTargetSelection::parameters
    \brief The values of the parameters, by their ids.
*/

/*!
    Returns an item for \a filePath, or only the lines in \a range of it.
*/
AiContextItem AiContextItem::fromFile(const FilePath &filePath,
                                      const std::optional<Text::Range> &range)
{
    AiContextItem item;
    item.kind = File;
    item.name = filePath.fileName();
    item.filePath = filePath;
    item.range = range;
    return item;
}

/*!
    Returns an item for \a text, which is shown as \a name.
*/
AiContextItem AiContextItem::fromText(const QString &name, const QString &text)
{
    AiContextItem item;
    item.kind = Text;
    item.name = name;
    item.text = text;
    return item;
}

/*!
    Returns an item for \a image, which is shown as \a name.
*/
AiContextItem AiContextItem::fromImage(const QString &name, const QImage &image)
{
    AiContextItem item;
    item.kind = Image;
    item.name = name;
    item.image = image;
    return item;
}

// AiResponse

/*!
    \class Utils::AiResponse
    \inmodule QtCreator

    \brief The AiResponse class follows one submitted AiRequest.

    finished() is emitted exactly once. For an interactive request it reports
    an empty string once the request was handed over, for example when the
    chat showing it was opened. For a headless request it reports the whole
    answer, after textReceived() delivered it in pieces.

    The response deletes itself after finished(). It is never emitted before
    the event loop ran, so connecting right after submit() is in time.
*/

/*!
    \internal
*/
/*!
    \fn void Utils::AiResponse::textReceived(const QString &chunk)

    Reports the next \a chunk of the answer of a headless request.
*/

/*!
    \fn void Utils::AiResponse::finished(const Utils::Result<QString> &result)

    Reports the end of the request with \a result: the whole answer of a
    headless request, an empty string for an interactive one, or an error.
*/

/*!
    \fn void Utils::AiResponse::cancelRequested()

    Asks the provider to stop working on the request.
*/

AiResponse::AiResponse()
    : QObject(shutdownGuard())
{}

AiResponse::~AiResponse() = default;

/*!
    Returns whether finished() was emitted.
*/
bool AiResponse::isFinished() const
{
    return m_finished;
}

/*!
    Asks the provider to stop. The response finishes with an error right
    away, whatever the provider reports afterwards.
*/
void AiResponse::cancel()
{
    if (m_finished)
        return;
    emit cancelRequested();
    reportFinished(ResultError(Tr::tr("The request was canceled.")));
}

/*!
    Reports the next \a chunk of the answer. For providers.
*/
void AiResponse::reportText(const QString &chunk)
{
    if (m_finished || chunk.isEmpty())
        return;
    emit textReceived(chunk);
}

/*!
    Reports the end of the request with \a result, the whole answer or an
    error. Later calls are ignored. For providers.
*/
void AiResponse::reportFinished(const Result<QString> &result)
{
    if (m_finished)
        return;
    m_finished = true;
    emit finished(result);
    deleteLater();
}

// Registry

class ProviderRegistry
{
public:
    QList<AiProvider *> providers;
    QList<std::pair<QPointer<QObject>, std::function<void()>>> callbacks;

    void notify()
    {
        Utils::erase(callbacks, [](const auto &entry) { return entry.first.isNull(); });
        const auto current = callbacks;
        for (const auto &[guard, callback] : current) {
            if (guard)
                callback();
        }
    }
};

static ProviderRegistry &registry()
{
    static ProviderRegistry theRegistry;
    return theRegistry;
}

class ClipboardAiProvider final : public AiProvider
{
public:
    ClipboardAiProvider()
        : AiProvider(AiAssistance::clipboardProviderId(), Tr::tr("Clipboard"))
    {
        AiTarget target;
        target.id = QLatin1String(clipboardTargetId);
        target.displayName = Tr::tr("Copy to Clipboard");
        target.description = Tr::tr("Copies the prompt and its context as text, to paste "
                                    "into an agent of your choice.");
        target.icon = Icons::COPY.icon();
        setTargets({target});
    }

    void execute(const AiRequest &request, const AiTargetSelection &, AiResponse *response) final
    {
        auto mimeData = new QMimeData;
        mimeData->setText(AiAssistance::toPlainText(request));
        const auto image = Utils::findOrDefault(request.context, [](const AiContextItem &item) {
            return item.kind == AiContextItem::Image && !item.image.isNull();
        });
        if (image.kind == AiContextItem::Image)
            mimeData->setImageData(image.image);
        QGuiApplication::clipboard()->setMimeData(mimeData);
        response->reportFinished(QString());
    }
};

static void ensureBuiltinProviders()
{
    // Creating a provider notifies, and whoever is notified may ask for the providers.
    static bool created = false;
    if (std::exchange(created, true))
        return;
    static GuardedObject<ClipboardAiProvider> clipboardProvider;
}

// AiProvider

/*!
    \class Utils::AiProvider
    \inmodule QtCreator

    \brief The AiProvider class is the base class for plugins that can carry
    out an AiRequest.

    A provider registers itself on construction and unregisters on destruction.
    Its targets can change at any time, for example when a provider learns
    which models an agent offers only after talking to it.
*/

/*!
    \fn void Utils::AiProvider::targetsChanged()

    This signal is emitted when the targets of the provider changed.
*/

/*!
    Creates and registers a provider with the unique \a id, which is shown as
    \a displayName.
*/
AiProvider::AiProvider(Id id, const QString &displayName)
    : m_id(id)
    , m_displayName(displayName)
{
    QTC_CHECK(id.isValid());
    QTC_CHECK(!AiAssistance::provider(id));
    registry().providers.append(this);
    registry().notify();
}

/*!
    Unregisters the provider.
*/
AiProvider::~AiProvider()
{
    registry().providers.removeOne(this);
    registry().notify();
}

/*!
    Returns the id of the provider.
*/
Id AiProvider::id() const
{
    return m_id;
}

/*!
    Returns the name of the provider that the user sees.
*/
QString AiProvider::displayName() const
{
    return m_displayName;
}

/*!
    Returns the targets of the provider.
*/
QList<AiTarget> AiProvider::targets() const
{
    return m_targets;
}

/*!
    Returns the target with \a targetId, if the provider has it.
*/
std::optional<AiTarget> AiProvider::target(const QString &targetId) const
{
    for (const AiTarget &target : m_targets) {
        if (target.id == targetId)
            return target;
    }
    return std::nullopt;
}

/*!
    \fn void Utils::AiProvider::execute(const AiRequest &request, const AiTargetSelection &selection, AiResponse *response)

    Carries out \a request on the target and with the parameters in
    \a selection. Every parameter of the target has a value in \a selection.

    The provider must eventually call AiResponse::reportFinished() on
    \a response, and should stop working on the request when
    AiResponse::cancelRequested() is emitted.
*/

void AiProvider::setTargets(const QList<AiTarget> &targets)
{
    m_targets = targets;
    emit targetsChanged();
    registry().notify();
}

// AiRequestRunner

/*!
    \class Utils::AiRequestRunner
    \inmodule QtCreator

    \brief The AiRequestRunner class runs an AiRequest as a task, which
    AiRequestTask puts into a task tree.

    Without a selection, the user picks the target. The request counts as
    successful when it finishes with an answer, which result() returns. A
    runner that is destroyed while its request runs, such as by stopping its
    task tree, cancels the request.
*/

/*!
    \typedef Utils::AiRequestTask
    \relates Utils::AiRequestRunner

    A task that runs an AiRequestRunner in a task tree.
*/

/*!
    \fn void Utils::AiRequestRunner::textReceived(const QString &chunk)

    Reports the next \a chunk of the answer of a headless request.
*/

/*!
    \fn void Utils::AiRequestRunner::done(QtTaskTree::DoneResult result)

    Reports the end of the request with \a result, which is a success if there
    is an answer.
*/

/*!
    Creates a runner without a request.
*/
AiRequestRunner::AiRequestRunner()
    : m_result(ResultError(Tr::tr("The request did not run.")))
{}

/*!
    Cancels the request if it runs.
*/
AiRequestRunner::~AiRequestRunner()
{
    if (m_response) {
        disconnect(m_response, nullptr, this, nullptr);
        m_response->cancel();
    }
}

/*!
    Sets the \a request to run.
*/
void AiRequestRunner::setRequest(const AiRequest &request)
{
    m_request = request;
}

/*!
    Runs the request on the target in \a selection, without asking the user.
*/
void AiRequestRunner::setSelection(const AiTargetSelection &selection)
{
    m_selection = selection;
}

/*!
    Submits the request.
*/
void AiRequestRunner::start()
{
    QTC_ASSERT(!m_response, return);
    m_response = m_selection ? AiAssistance::submit(m_request, *m_selection)
                             : AiAssistance::submit(m_request);
    connect(m_response, &AiResponse::textReceived, this, &AiRequestRunner::textReceived);
    connect(m_response, &AiResponse::finished, this, [this](const Result<QString> &result) {
        m_result = result;
        m_response.clear();
        emit done(QtTaskTree::toDoneResult(result.has_value()));
    });
}

/*!
    Returns the answer, or an error if the request failed or has not finished.
*/
Result<QString> AiRequestRunner::result() const
{
    return m_result;
}

namespace AiAssistance {

/*!
    Returns the registered providers. The clipboard provider comes last.
*/
QList<AiProvider *> providers()
{
    ensureBuiltinProviders();
    QList<AiProvider *> result = registry().providers;
    std::stable_partition(result.begin(), result.end(), [](AiProvider *provider) {
        return provider->id() != clipboardProviderId();
    });
    return result;
}

/*!
    Returns the provider with \a providerId, if it is registered.
*/
AiProvider *provider(Id providerId)
{
    return Utils::findOrDefault(registry().providers, [providerId](AiProvider *provider) {
        return provider->id() == providerId;
    });
}

static bool targetFits(const AiTarget &target, AiRequest::Mode mode)
{
    return mode == AiRequest::Interactive || target.supportsHeadless;
}

/*!
    Returns whether a target can run requests in \a mode.
*/
bool hasTargets(AiRequest::Mode mode)
{
    for (AiProvider *provider : providers()) {
        for (const AiTarget &target : provider->targets()) {
            if (targetFits(target, mode))
                return true;
        }
    }
    return false;
}

/*!
    Calls \a callback whenever a provider is added or removed or changes its
    targets, until \a guard is destroyed.
*/
void onTargetsChanged(QObject *guard, const std::function<void()> &callback)
{
    QTC_ASSERT(guard && callback, return);
    registry().callbacks.append({guard, callback});
}

static Key selectionKey(Id requestId)
{
    return Key(settingsGroup) + '/'
           + (requestId.isValid() ? requestId.toKey() : Key(lastSelectionKey));
}

static std::optional<AiTargetSelection> readSelection(const Key &key)
{
    const Store store = storeFromVariant(userSettings().value(key));
    if (store.isEmpty())
        return std::nullopt;
    AiTargetSelection selection;
    selection.providerId = Id::fromSetting(store.value(providerKey));
    selection.targetId = store.value(targetKey).toString();
    const Store parameters = storeFromVariant(store.value(parametersKey));
    for (auto it = parameters.cbegin(); it != parameters.cend(); ++it)
        selection.parameters.insert(stringFromKey(it.key()), it.value().toString());
    if (!selection.providerId.isValid() || selection.targetId.isEmpty())
        return std::nullopt;
    return selection;
}

static void rememberSelection(Id requestId, const AiTargetSelection &selection)
{
    Store store;
    store.insert(providerKey, selection.providerId.toSetting());
    store.insert(targetKey, selection.targetId);
    Store parameters;
    for (auto it = selection.parameters.cbegin(); it != selection.parameters.cend(); ++it)
        parameters.insert(keyFromString(it.key()), it.value());
    store.insert(parametersKey, variantFromStore(parameters));
    const QVariant value = variantFromStore(store);
    if (requestId.isValid())
        userSettings().setValue(selectionKey(requestId), value);
    userSettings().setValue(selectionKey({}), value);
}

/*!
    Returns the target the user picked the last time a request with
    \a requestId was submitted, or for any request if there was none with that
    id yet.
*/
std::optional<AiTargetSelection> rememberedSelection(Id requestId)
{
    if (requestId.isValid()) {
        if (const auto selection = readSelection(selectionKey(requestId)))
            return selection;
    }
    return readSelection(selectionKey({}));
}

static Result<AiTargetSelection> completeSelection(const AiRequest &request,
                                                   const AiTargetSelection &selection)
{
    AiProvider *provider = AiAssistance::provider(selection.providerId);
    if (!provider) {
        return ResultError(Tr::tr("The AI provider \"%1\" is not available.")
                               .arg(selection.providerId.toString()));
    }
    const std::optional<AiTarget> target = provider->target(selection.targetId);
    if (!target) {
        return ResultError(Tr::tr("\"%1\" does not offer the target \"%2\".")
                               .arg(provider->displayName(), selection.targetId));
    }
    if (!targetFits(*target, request.mode)) {
        return ResultError(Tr::tr("\"%1\" cannot return an answer to Qt Creator.")
                               .arg(target->displayName));
    }
    AiTargetSelection result = selection;
    for (const AiTargetParameter &parameter : target->parameters) {
        if (!result.parameters.contains(parameter.id))
            result.parameters.insert(parameter.id, parameter.defaultValue);
    }
    return result;
}

static void execute(const AiRequest &request, const AiTargetSelection &selection,
                    AiResponse *response)
{
    const Result<AiTargetSelection> complete = completeSelection(request, selection);
    if (!complete) {
        response->reportFinished(ResultError(complete.error()));
        return;
    }
    provider(complete->providerId)->execute(request, *complete, response);
}

/*!
    Carries out \a request on the target in \a selection, without asking the
    user.
*/
AiResponse *submit(const AiRequest &request, const AiTargetSelection &selection)
{
    ensureBuiltinProviders();
    auto response = new AiResponse;
    QTimer::singleShot(0, response, [request, selection, response] {
        if (!response->isFinished())
            execute(request, selection, response);
    });
    return response;
}

class AiTargetDialog : public QDialog
{
public:
    AiTargetDialog(const AiRequest &request, QWidget *parent)
        : QDialog(parent)
        , m_mode(request.mode)
    {
        setWindowTitle(request.title.isEmpty() ? Tr::tr("Send to AI") : request.title);

        auto label = new QLabel(Tr::tr("Send the request to:"));
        m_targetList = new QListWidget;
        m_parameterLayout = new QFormLayout;
        m_parameterLayout->setContentsMargins(0, 0, 0, 0);
        m_parameterLayout->setHorizontalSpacing(StyleHelper::SpacingTokens::GapHM);
        m_parameterLayout->setVerticalSpacing(StyleHelper::SpacingTokens::GapVS);
        m_buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        m_buttons->button(QDialogButtonBox::Ok)->setText(Tr::tr("Send"));

        auto layout = new QVBoxLayout(this);
        layout->setSpacing(StyleHelper::SpacingTokens::GapVM);
        layout->addWidget(label);
        layout->addWidget(m_targetList);
        layout->addLayout(m_parameterLayout);
        layout->addWidget(m_buttons);

        connect(m_buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(m_buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
        connect(m_targetList, &QListWidget::currentRowChanged, this, [this] {
            updateParameters({});
        });
        connect(m_targetList, &QListWidget::itemDoubleClicked, this, &QDialog::accept);
        onTargetsChanged(this, [this] { populate(selection()); });

        populate(rememberedSelection(request.id));
    }

    std::optional<AiTargetSelection> selection() const
    {
        const QListWidgetItem *item = m_targetList->currentItem();
        if (!item)
            return std::nullopt;
        AiTargetSelection result;
        result.providerId = Id::fromSetting(item->data(ProviderRole));
        result.targetId = item->data(TargetRole).toString();
        for (QComboBox *combo : m_parameterCombos)
            result.parameters.insert(combo->property(parameterIdProperty).toString(),
                                     combo->currentData().toString());
        return result;
    }

private:
    enum { ProviderRole = Qt::UserRole, TargetRole };
    static constexpr char parameterIdProperty[] = "aiParameterId";

    void populate(const std::optional<AiTargetSelection> &preferred)
    {
        m_targetList->blockSignals(true);
        m_targetList->clear();
        int preferredRow = 0;
        for (AiProvider *provider : providers()) {
            for (const AiTarget &target : provider->targets()) {
                if (!targetFits(target, m_mode))
                    continue;
                auto item = new QListWidgetItem(target.icon, target.displayName, m_targetList);
                item->setToolTip(target.description);
                item->setData(ProviderRole, provider->id().toSetting());
                item->setData(TargetRole, target.id);
                if (preferred && preferred->providerId == provider->id()
                    && preferred->targetId == target.id) {
                    preferredRow = m_targetList->count() - 1;
                }
            }
        }
        m_targetList->setCurrentRow(m_targetList->count() > 0 ? preferredRow : -1);
        m_targetList->blockSignals(false);
        updateParameters(preferred ? preferred->parameters : QMap<QString, QString>());
    }

    void updateParameters(const QMap<QString, QString> &preferred)
    {
        while (m_parameterLayout->rowCount() > 0)
            m_parameterLayout->removeRow(0);
        m_parameterCombos.clear();

        const std::optional<AiTargetSelection> current = selection();
        m_buttons->button(QDialogButtonBox::Ok)->setEnabled(current.has_value());
        if (!current)
            return;
        AiProvider *provider = AiAssistance::provider(current->providerId);
        QTC_ASSERT(provider, return);
        const std::optional<AiTarget> target = provider->target(current->targetId);
        QTC_ASSERT(target, return);

        for (const AiTargetParameter &parameter : target->parameters) {
            auto combo = new QComboBox;
            combo->setProperty(parameterIdProperty, parameter.id);
            for (const AiParameterChoice &choice : parameter.choices) {
                combo->addItem(choice.displayName, choice.value);
                combo->setItemData(combo->count() - 1, choice.description, Qt::ToolTipRole);
            }
            const QString value = preferred.value(parameter.id, parameter.defaultValue);
            combo->setCurrentIndex(std::max(0, combo->findData(value)));
            m_parameterLayout->addRow(parameter.displayName + ':', combo);
            m_parameterCombos.append(combo);
        }
    }

    AiRequest::Mode m_mode;
    QListWidget *m_targetList = nullptr;
    QFormLayout *m_parameterLayout = nullptr;
    QDialogButtonBox *m_buttons = nullptr;
    QList<QComboBox *> m_parameterCombos;
};

/*!
    Lets the user pick a target for \a request, with the target picked last
    time for the same request id preselected, and carries it out there.

    If the user cancels, the response finishes with an error.
*/
AiResponse *submit(const AiRequest &request)
{
    ensureBuiltinProviders();
    auto response = new AiResponse;
    auto dialog = new AiTargetDialog(request, dialogParent());
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    QObject::connect(response, &AiResponse::finished, dialog, &QDialog::reject);
    QObject::connect(dialog, &QDialog::finished, response, [dialog, request, response](int r) {
        if (response->isFinished())
            return;
        const std::optional<AiTargetSelection> selection = dialog->selection();
        if (r != QDialog::Accepted || !selection) {
            response->reportFinished(ResultError(Tr::tr("No AI target was selected.")));
            return;
        }
        rememberSelection(request.id, *selection);
        execute(request, *selection, response);
    });
    dialog->open();
    return response;
}

/*!
    Creates an action with \a text and \a parent that submits the request
    \a request returns when triggered. The action is disabled while no target
    is available.
*/
QAction *createAction(const QString &text, const std::function<AiRequest()> &request,
                      QObject *parent)
{
    auto action = new QAction(text, parent);
    const auto update = [action] { action->setEnabled(hasTargets(AiRequest::Interactive)); };
    update();
    onTargetsChanged(action, update);
    QObject::connect(action, &QAction::triggered, action, [request] { submit(request()); });
    return action;
}

static QString fenced(const QString &text)
{
    QString fence = "```";
    while (text.contains(fence))
        fence += '`';
    return fence + '\n' + text + (text.endsWith('\n') ? "" : "\n") + fence + '\n';
}

/*!
    Returns the lines of \a filePath that \a range covers, from its first to
    its last line, or an empty string if the file cannot be read.
*/
QString excerpt(const FilePath &filePath, const Text::Range &range)
{
    const Result<QByteArray> contents = filePath.fileContents();
    if (!contents)
        return {};
    const QStringList lines = QString::fromUtf8(*contents).split('\n');
    const int first = std::max(1, range.begin.line);
    const int last = std::min(int(lines.size()), std::max(first, range.end.line));
    return lines.mid(first - 1, last - first + 1).join('\n');
}

/*!
    Returns \a request as text for an agent that gets nothing else: the
    prompt, followed by the context. Files are referenced by path, with the
    lines a range covers quoted. Images cannot be put into text and are only
    named.
*/
QString toPlainText(const AiRequest &request)
{
    QString result = request.prompt;
    if (!result.endsWith('\n'))
        result += '\n';
    if (!request.workingDirectory.isEmpty()) {
        result += '\n' + Tr::tr("Working directory: %1")
                             .arg(request.workingDirectory.toUserOutput()) + '\n';
    }
    for (const AiContextItem &item : request.context) {
        result += '\n';
        switch (item.kind) {
        case AiContextItem::File:
            if (item.range) {
                result += Tr::tr("File: %1, lines %2-%3")
                              .arg(item.filePath.toUserOutput())
                              .arg(item.range->begin.line)
                              .arg(item.range->end.line) + '\n';
                const QString text = excerpt(item.filePath, *item.range);
                if (!text.isEmpty())
                    result += fenced(text);
            } else {
                result += Tr::tr("File: %1").arg(item.filePath.toUserOutput()) + '\n';
            }
            break;
        case AiContextItem::Text:
            result += item.name + ":\n" + fenced(item.text);
            break;
        case AiContextItem::Image:
            result += Tr::tr("Image \"%1\" (not included in text)").arg(item.name) + '\n';
            break;
        }
    }
    return result;
}

/*!
    Returns the id of the built-in provider that copies requests to the
    clipboard.
*/
Id clipboardProviderId()
{
    return "Utils.AiAssistance.Clipboard";
}

} // namespace AiAssistance

} // namespace Utils
