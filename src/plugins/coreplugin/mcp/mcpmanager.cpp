// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mcpmanager.h"

#include "../coreconstants.h"
#include "../coreplugintr.h"
#include "../dialogs/ioptionspage.h"
#include "../icore.h"
#include "mcpregistry.h"

#include <solutions/spinner/spinner.h>

#include <utils/algorithm.h>
#include <utils/aspectlist.h>
#include <utils/aspects.h>
#include <utils/co_result.h>
#include <utils/environmentchangesaspect.h>
#include <utils/async.h>
#include <utils/layoutbuilder.h>
#include <utils/networkaccessmanager.h>
#include <utils/shutdownguard.h>
#include <utils/theme/theme.h>

#include <QComboBox>
#include <QCryptographicHash>
#include <QDialog>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QIcon>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QMetaEnum>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>
#include <QStringListModel>
#include <QTimer>
#include <QUuid>
#include <QVBoxLayout>

#include <QtTaskTree/QNetworkReplyWrapper>
#include <QtTaskTree/QParallelTaskTreeRunner>

using namespace Utils;

namespace Core {

static const char MCPMANAGER_SETTINGS_PAGE_ID[] = "AI.MCPMANAGER";
static const char QT_DOCS_MCPSERVER_MANAGER_ID[] = "QTCREATOR.BUILTIN.QTDOCS.MCP.SERVER";
// --- MCP Registry Download & Cache ---

static const char MCP_REGISTRY_URL[] = "https://qtccache.qt.io/mcp/registry.json";

// --- Icon Cache ---

static Result<> writeMcpIconFile(const QByteArray &data, const FilePath &destPath)
{
    co_await destPath.parentDir().ensureWritableDir();
    co_await destPath.writeFileContents(data);
    co_return ResultOk;
}

static QString iconExtension(const McpRegistry::Icon &icon)
{
    if (icon.mime_type().has_value()) {
        const QString &mime = *icon.mime_type();
        if (mime.contains("svg"))
            return ".svg";
        if (mime.contains("png"))
            return ".png";
        if (mime.contains("jpeg") || mime.contains("jpg"))
            return ".jpg";
        if (mime.contains("webp"))
            return ".webp";
        if (mime.contains("gif"))
            return ".gif";
    }
    // Fall back to URL extension
    const QString url = icon.url();
    const int dot = url.lastIndexOf('.');
    if (dot >= 0) {
        const int end = url.indexOf('?', dot);
        return url.mid(dot, end > dot ? end - dot : -1).toLower();
    }
    return ".png"; // safe default
}

static bool isDarkTheme()
{
    return creatorTheme() && creatorTheme()->flag(Theme::DarkUserInterface);
}

// Pick the best icon for the current theme: prefer a matching theme variant,
// then fall back to any icon without a theme annotation.
static const McpRegistry::Icon *bestIconForTheme(const std::optional<QList<McpRegistry::Icon>> &icons)
{
    if (!icons.has_value())
        return nullptr;
    const auto preferred = isDarkTheme() ? McpRegistry::Icon::Theme::dark
                                         : McpRegistry::Icon::Theme::light;

    const McpRegistry::Icon *themeMatch = nullptr;
    const McpRegistry::Icon *fallback = nullptr;

    for (const auto &icon : *icons) {
        if (icon.theme().has_value()) {
            if (*icon.theme() == preferred && !themeMatch)
                themeMatch = &icon;
        } else {
            if (!fallback)
                fallback = &icon;
        }
    }
    if (themeMatch)
        return themeMatch;
    if (fallback)
        return fallback;
    return icons->isEmpty() ? nullptr : &icons->first();
}

static FilePath mcpIconCachePath(const McpRegistry::Icon &icon)
{
    const QByteArray hash = QCryptographicHash::hash(icon.url().toUtf8(), QCryptographicHash::Sha1);
    return ICore::userResourcePath("mcpregistry/icons")
           / (QString::fromLatin1(hash.toHex()) + iconExtension(icon));
}

// Returns a cached icon synchronously if available (from embedded data or prior download).
static QIcon fetchMcpIconFromCache(const McpRegistry::Server &server)
{
    const auto *icon = bestIconForTheme(server.icons());
    if (!icon)
        return {};

    const FilePath destPath = mcpIconCachePath(*icon);

    if (destPath.isFile())
        return QIcon(destPath.toFSPathString());

    if (icon->data().has_value()) {
        QByteArray data = QByteArray::fromBase64(icon->data()->toUtf8());
        Result<> res = writeMcpIconFile(data, destPath);
        if (res)
            return QIcon(destPath.toFSPathString());
    }
    return {};
}

// --- MCP Registry Browser Model & Dialog ---

class McpRegistryModel : public QAbstractListModel
{
    Q_OBJECT
public:
    static const int SortRole = Qt::UserRole + 1;
    static const int TypeTagsRole = Qt::UserRole + 2;

    explicit McpRegistryModel(QObject *parent = nullptr)
        : QAbstractListModel(parent)
    {
        QTimer::singleShot(0, this, &McpRegistryModel::fetchMcpRegistry);
    }

    static McpRegistryModel &instance()
    {
        static GuardedObject<McpRegistryModel> inst;
        return inst;
    }

    void fetchMcpRegistry()
    {
        using namespace QtTaskTree;

        Storage<QByteArray> registryData;
        Storage<QString> errorString;

        const auto setupFetch = [](QNetworkReplyWrapper &wrapper) {
            wrapper.setNetworkAccessManager(NetworkAccessManager::instance());
            wrapper.setOperation(QNetworkAccessManager::GetOperation);
            wrapper.setRequest(QNetworkRequest(QUrl(MCP_REGISTRY_URL)));
        };

        const auto fetchDone = [registryData, errorString](const QNetworkReplyWrapper &wrapper) {
            if (wrapper.reply()->error() != QNetworkReply::NoError) {
                *errorString = wrapper.reply()->errorString();
                return;
            }
            *registryData = wrapper.reply()->readAll();
        };

        const auto setupParsing = [registryData](Async<Result<QList<McpRegistry::Server>>> &async) {
            async.setConcurrentCallData(
                [](QPromise<Result<QList<McpRegistry::Server>>> &promise, const QByteArray &data) {
                    QJsonParseError error;
                    const QJsonDocument doc = QJsonDocument::fromJson(data, &error);
                    if (error.error != QJsonParseError::NoError) {
                        promise.addResult(
                            ResultError(QString("Failed to parse MCP registry JSON: %1")
                                            .arg(error.errorString())));
                        return;
                    }

                    Utils::Result<McpRegistry::McpRegistry> registry
                        = McpRegistry::fromJson<McpRegistry::McpRegistry>(doc.object());
                    if (!registry) {
                        promise.addResult(ResultError(
                            QString("Failed to parse MCP registry: %1").arg(registry.error())));
                        return;
                    }

                    promise.addResult(
                        Utils::filtered(registry->servers(), [](const McpRegistry::Server &server) {
                            return server.status() != McpRegistry::Server::Status::deleted;
                        }));
                },
                *registryData);
        };

        const auto parsingDone =
            [this, errorString](const Async<Result<QList<McpRegistry::Server>>> &task) {
                auto result = task.takeResult();
                if (!result) {
                    *errorString = result.error();
                    return;
                }
                beginInsertRows(QModelIndex(), 0, result->count() - 1);
                m_mcpServers = std::move(*result);
                endInsertRows();
            };

        m_taskTreeRunner.start({
            registryData,
            errorString,
            onGroupSetup([this, errorString]() {
                *errorString = QString();
                emit isLoadingChanged(true);
                emit errorStringChanged(*errorString);
            }),
            onGroupDone([this, errorString]() {
                emit isLoadingChanged(false);
                emit errorStringChanged(*errorString);
            }),
            QNetworkReplyWrapperTask(setupFetch, fetchDone),
            AsyncTask<Result<QList<McpRegistry::Server>>>(setupParsing, parsingDone),
        });
    }

    void setIcon(const QIcon &icon, int row) const
    {
        if (!icon.isNull()) {
            m_icons[row] = icon;
            const QModelIndex idx = index(row);
            emit const_cast<McpRegistryModel *>(this)->dataChanged(idx, idx, {Qt::DecorationRole});
        }
    }

    // Downloads the best icon asynchronously, calling onDone with the resulting QIcon.
    void fetchMcpIconAsync(const McpRegistry::Server &server, int row) const
    {
        using namespace QtTaskTree;

        const McpRegistry::Icon *icon = bestIconForTheme(server.icons());
        if (!icon || icon->url().isEmpty())
            return;

        const FilePath destPath = mcpIconCachePath(*icon);
        if (destPath.isFile()) {
            setIcon(QIcon(destPath.toFSPathString()), row);
            return;
        }

        const QString url = icon->url();
        const auto setupFetch = [url](QNetworkReplyWrapper &task) {
            task.setNetworkAccessManager(Utils::NetworkAccessManager::instance());
            task.setOperation(QNetworkAccessManager::GetOperation);
            task.setRequest(QNetworkRequest(url));
        };

        const auto fetchDone =
            [this, row, destPath](const QNetworkReplyWrapper &task, DoneWith doneWith) {
                if (doneWith != DoneWith::Success)
                    return;

                Result<> res = writeMcpIconFile(task.reply()->readAll(), destPath);
                if (res)
                    setIcon(QIcon(destPath.toFSPathString()), row);
            };

        m_taskTreeRunner.start({QNetworkReplyWrapperTask(setupFetch, fetchDone)});
    }

    int rowCount(const QModelIndex & = {}) const override { return m_mcpServers.size(); }

    QVariant data(const QModelIndex &index, int role) const override
    {
        if (!index.isValid() || index.row() < 0 || index.row() >= m_mcpServers.size())
            return {};

        const McpRegistry::Server &server = m_mcpServers.at(index.row());

        switch (role) {
        case Qt::DisplayRole:
            return server.title().value_or(server.name());
        case Qt::ToolTipRole:
            return server.description();
        case Qt::DecorationRole:
            return iconForRow(index.row());
        case SortRole: {
            const auto *iconEntry = bestIconForTheme(server.icons());
            const bool hasIcon = iconEntry
                                 && (!iconEntry->url().isEmpty() || iconEntry->data().has_value());
            return hasIcon ? 0 : 1;
        }
        case TypeTagsRole:
            return QVariant::fromValue(typeTags(server));
        default:
            return {};
        }
    }

    const McpRegistry::Server *serverAt(int row) const
    {
        if (row < 0 || row >= m_mcpServers.size())
            return nullptr;
        return &m_mcpServers.at(row);
    }

private:
    QIcon iconForRow(int row) const
    {
        auto &cached = m_icons[row];
        if (cached.has_value())
            return *cached;

        const McpRegistry::Server &server = m_mcpServers.at(row);
        QIcon icon = fetchMcpIconFromCache(server);
        if (!icon.isNull()) {
            cached = icon;
            return icon;
        }

        // Mark as attempted (empty icon) so we don't re-fetch
        cached = QIcon();
        fetchMcpIconAsync(server, row);
        return {};
    }

    static QStringList typeTags(const McpRegistry::Server &server)
    {
        QStringList tags;
        for (const auto &pkg : server.packages().value_or(QList<McpRegistry::Package>{})) {
            switch (pkg.registry_type()) {
            case McpRegistry::Package::Registry_type::npm:
                if (!tags.contains(QString("npm")))
                    tags.append(QString("npm"));
                break;
            case McpRegistry::Package::Registry_type::pypi:
                if (!tags.contains(QString("pypi")))
                    tags.append(QString("pypi"));
                break;
            case McpRegistry::Package::Registry_type::nuget:
                if (!tags.contains(QString("nuget")))
                    tags.append(QString("nuget"));
                break;
            case McpRegistry::Package::Registry_type::oci:
                if (!tags.contains(QString("oci")))
                    tags.append(QString("oci"));
                break;
            default:
                break;
            }
        }
        if (server.remotes().has_value() && !server.remotes()->isEmpty()
            && !tags.contains(QString("remote")))
            tags.append(QString("remote"));
        return tags;
    }

signals:
    void isLoadingChanged(bool isLoading);
    void errorStringChanged(const QString &errorString);

private:
    mutable QHash<int, std::optional<QIcon>> m_icons;
    mutable QtTaskTree::QParallelTaskTreeRunner m_taskTreeRunner;
    QList<McpRegistry::Server> m_mcpServers;
};

class McpRegistryProxyModel : public QSortFilterProxyModel
{
public:
    using QSortFilterProxyModel::QSortFilterProxyModel;

    void setTextFilter(const QString &text)
    {
        m_text = text;
        invalidateFilter();
    }
    void setTypeFilter(const QString &type)
    {
        m_type = type;
        invalidateFilter();
    }

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override
    {
        const QModelIndex idx = sourceModel()->index(sourceRow, 0, sourceParent);
        if (!m_text.isEmpty()) {
            const QString display = idx.data(Qt::DisplayRole).toString();
            if (!display.contains(m_text, Qt::CaseInsensitive))
                return false;
        }
        if (!m_type.isEmpty()) {
            const QStringList tags = idx.data(McpRegistryModel::TypeTagsRole).toStringList();
            if (!tags.contains(m_type))
                return false;
        }
        return true;
    }

private:
    QString m_text;
    QString m_type;
};

class McpRegistryDialog : public QDialog
{
public:
    McpRegistryDialog(QWidget *parent = nullptr)
        : QDialog(parent)
    {
        setWindowTitle(Tr::tr("Add MCP Server From Registry"));
        resize(700, 500);

        auto *layout = new QVBoxLayout(this);

        m_errorLabel = new QLabel(this);
        m_errorLabel->setWordWrap(true);
        m_errorLabel->hide();

        connect(
            &McpRegistryModel::instance(),
            &McpRegistryModel::errorStringChanged,
            this,
            [this](const QString &errorString) {
                if (errorString.isEmpty()) {
                    m_errorLabel->hide();
                } else {
                    m_errorLabel->setText(errorString);
                    m_errorLabel->show();
                }
            },
            Qt::QueuedConnection);

        auto *filterRow = new QHBoxLayout;

        m_filterEdit = new QLineEdit(this);
        m_filterEdit->setPlaceholderText(Tr::tr("Filter servers..."));
        m_filterEdit->setClearButtonEnabled(true);
        filterRow->addWidget(m_filterEdit, 1);

        m_typeCombo = new QComboBox(this);
        m_typeCombo->addItem(Tr::tr("All Types"));
        m_typeCombo->addItem(Tr::tr("npm"), QString("npm"));
        m_typeCombo->addItem(Tr::tr("PyPI"), QString("pypi"));
        m_typeCombo->addItem(Tr::tr("NuGet"), QString("nuget"));
        m_typeCombo->addItem(Tr::tr("OCI"), QString("oci"));
        m_typeCombo->addItem(Tr::tr("Remote"), QString("remote"));
        filterRow->addWidget(m_typeCombo);

        layout->addLayout(filterRow);

        auto *contentRow = new QHBoxLayout;

        m_listView = new QListView(this);
        m_listView->setEditTriggers(QAbstractItemView::NoEditTriggers);

        contentRow->addWidget(m_listView, 1);

        auto progressOverlay
            = new SpinnerSolution::Spinner(SpinnerSolution::SpinnerSize::Large, this);
        progressOverlay->setColor(Qt::white);
        progressOverlay->hide();
        connect(
            &McpRegistryModel::instance(),
            &McpRegistryModel::isLoadingChanged,
            progressOverlay,
            [progressOverlay, this](bool isLoading) {
                setEnabled(!isLoading);
                if (isLoading)
                    progressOverlay->show();
                else
                    progressOverlay->hide();
            },
            Qt::QueuedConnection);

        m_detailsLabel = new QLabel(this);
        m_detailsLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);
        m_detailsLabel->setWordWrap(true);
        m_detailsLabel->setTextFormat(Qt::RichText);
        m_detailsLabel->setOpenExternalLinks(true);
        m_detailsLabel->setMargin(8);
        m_detailsLabel->setMinimumWidth(250);
        m_detailsLabel->hide();
        contentRow->addWidget(m_detailsLabel, 1);

        layout->addLayout(contentRow, 1);
        layout->addWidget(m_errorLabel);

        m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
        m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
        layout->addWidget(m_buttonBox);

        m_proxyModel = new McpRegistryProxyModel(this);
        m_proxyModel->setSourceModel(&McpRegistryModel::instance());
        m_proxyModel->setSortRole(McpRegistryModel::SortRole);
        m_listView->setModel(m_proxyModel);

        m_proxyModel->sort(0);

        connect(
            m_filterEdit,
            &QLineEdit::textChanged,
            m_proxyModel,
            &McpRegistryProxyModel::setTextFilter);

        connect(m_typeCombo, &QComboBox::currentIndexChanged, this, [this] {
            m_proxyModel->setTypeFilter(m_typeCombo->currentData().toString());
        });

        connect(
            m_listView->selectionModel(),
            &QItemSelectionModel::currentChanged,
            this,
            [this](const QModelIndex &current) {
                m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(current.isValid());
                updateDetails(current);
            });

        connect(m_listView, &QListView::doubleClicked, this, &QDialog::accept);
        connect(m_buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    }

    std::optional<McpRegistry::Server> selectedServer() const
    {
        const QModelIndex proxyIdx = m_listView->currentIndex();
        if (!proxyIdx.isValid())
            return {};
        const QModelIndex srcIdx = m_proxyModel->mapToSource(proxyIdx);
        const auto *server = McpRegistryModel::instance().serverAt(srcIdx.row());
        if (!server)
            return {};
        return *server;
    }

private:
    void updateDetails(const QModelIndex &proxyIdx)
    {
        if (!proxyIdx.isValid()) {
            m_detailsLabel->hide();
            return;
        }

        const QModelIndex srcIdx = m_proxyModel->mapToSource(proxyIdx);
        const auto *server = McpRegistryModel::instance().serverAt(srcIdx.row());
        if (!server) {
            m_detailsLabel->hide();
            return;
        }

        QString html;
        html += QString("<h3>%1</h3>").arg(server->title().value_or(server->name()).toHtmlEscaped());
        html += QString("<p>%1</p>").arg(server->description().toHtmlEscaped());

        html += QString("<p><b>%1</b> %2</p>")
                    .arg(Tr::tr("Version:"), server->version().toHtmlEscaped());

        if (server->repository_url().has_value()) {
            const QString &url = *server->repository_url();
            html += QString("<p><b>%1</b> <a href=\"%2\">%2</a></p>")
                        .arg(Tr::tr("Repository:"), url.toHtmlEscaped());
        }
        if (server->website_url().has_value()) {
            const QString &url = *server->website_url();
            html += QString("<p><b>%1</b> <a href=\"%2\">%2</a></p>")
                        .arg(Tr::tr("Website:"), url.toHtmlEscaped());
        }

        if (server->packages().has_value() && !server->packages()->isEmpty()) {
            html += QString("<p><b>%1</b></p><ul>").arg(Tr::tr("Packages:"));
            for (const auto &pkg : *server->packages()) {
                QString entry = pkg.identifier().toHtmlEscaped();
                if (pkg.version().has_value())
                    entry += QString("@") + pkg.version()->toHtmlEscaped();
                entry += QString(" <i>(%1, %2)</i>")
                             .arg(
                                 McpRegistry::toString(pkg.registry_type()).toHtmlEscaped(),
                                 McpRegistry::toString(pkg.transport_type()).toHtmlEscaped());
                html += QString("<li>%1</li>").arg(entry);
            }
            html += QString("</ul>");
        }

        if (server->remotes().has_value() && !server->remotes()->isEmpty()) {
            html += QString("<p><b>%1</b></p><ul>").arg(Tr::tr("Remotes:"));
            for (const auto &remote : *server->remotes()) {
                html += QString("<li>%1 <i>(%2)</i></li>")
                            .arg(
                                remote.url().toHtmlEscaped(),
                                McpRegistry::toString(remote.type()).toHtmlEscaped());
            }
            html += QString("</ul>");
        }

        m_detailsLabel->setText(html);
        m_detailsLabel->show();
    }

    QLineEdit *m_filterEdit = nullptr;
    QComboBox *m_typeCombo = nullptr;
    QListView *m_listView = nullptr;
    QLabel *m_detailsLabel = nullptr;
    QLabel *m_errorLabel = nullptr;
    QDialogButtonBox *m_buttonBox = nullptr;
    McpRegistryProxyModel *m_proxyModel = nullptr;
};

// --- McpServerAspect ---

class McpServerAspect : public AspectContainer
{
    static const auto &conTypeEnum()
    {
        static auto e = QMetaEnum::fromType<McpManager::ConnectionType>();
        return e;
    };

public:
    McpServerAspect()
    {
        setAutoApply(false);

        id.setSettingsKey("id");
        id.setValue(QUuid::createUuid().toString());

        name.setLabelText(Tr::tr("Name:"));
        name.setSettingsKey("name");
        name.setToolTip(Tr::tr("The display name."));
        name.setDisplayStyle(StringAspect::DisplayStyle::LineEditDisplay);
        name.setDefaultValue(Tr::tr("<New Server>"));

        // name.setValidationFunction([](){});

        launchCommand.setLabelText(Tr::tr("Executable:"));
        launchCommand.setSettingsKey("launchCommand");
        launchCommand.setToolTip(
            Tr::tr(
                "The command to launch the MCP server process. Only used for standard IO "
                "connection type."));
        launchCommand.setExpectedKind(PathChooser::ExistingCommand);

        launchArguments.setLabelText(Tr::tr("Arguments:"));
        launchArguments.setSettingsKey("launchArguments");
        launchArguments.setToolTip(
            Tr::tr(
                "The arguments to launch the MCP server process. Only used for standard IO "
                "connection type."));
        launchArguments.setDisplayStyle(StringAspect::DisplayStyle::LineEditDisplay);

        url.setLabelText("URL:");
        url.setSettingsKey("url");
        url.setToolTip(
            Tr::tr(
                "The URL to connect to the MCP server. Not used for standard IO connection type."));
        url.setDisplayStyle(StringAspect::DisplayStyle::LineEditDisplay);

        connectionType.setLabelText(Tr::tr("Connection type:"));
        connectionType.setSettingsKey("connectionType");
        connectionType.setToolTip(Tr::tr("The type of connection to use for the MCP server."));
        connectionType.setComboBoxEditable(false);
        connectionType.setFillCallback([](StringSelectionAspect::ResultCallback cb) {
            QStandardItem *stdItem = new QStandardItem(Tr::tr("Standard IO"));
            stdItem->setData(conTypeEnum().valueToKey(McpManager::Stdio));
            stdItem->setToolTip("Standard IO");
            QStandardItem *sseItem = new QStandardItem(Tr::tr("SSE"));
            sseItem->setData(conTypeEnum().valueToKey(McpManager::Sse));
            sseItem->setToolTip("SSE");
            QStandardItem *httpItem = new QStandardItem(Tr::tr("Streamable HTTP"));
            httpItem->setData(conTypeEnum().valueToKey(McpManager::Streamable_Http));
            httpItem->setToolTip("Streamable HTTP");
            cb({stdItem, sseItem, httpItem});
        });

        httpHeaders.setLabelText(Tr::tr("HTTP headers:"));
        httpHeaders.setSettingsKey("httpHeaders");
        httpHeaders.setToolTip(
            Tr::tr(
                "HTTP headers to include when connecting to the MCP server. Only used for HTTP "
                "connection types."));

        envChanges.setLabelText(Tr::tr("Environment:"));
        envChanges.setSettingsKey("envChanges");
        envChanges.setToolTip(
            Tr::tr(
                "Environment variable changes applied when launching the MCP server process. "
                "Only used for standard IO connection type."));

        setLayouter([this]() -> Layouting::Layout {
            using namespace Layouting;

            const auto updateVisible = [this]() {
                const QString type = connectionType.volatileValue();
                const bool isStdio = conTypeEnum().keyToValue(type.toUtf8()) == McpManager::Stdio;

                launchCommand.setVisible(isStdio);
                launchArguments.setVisible(isStdio);
                envChanges.setVisible(isStdio);
                url.setVisible(!isStdio);
                httpHeaders.setVisible(!isStdio);
            };
            updateVisible();

            connect(
                &connectionType, &StringSelectionAspect::volatileValueChanged, this, updateVisible);

            // clang-format off
            return Form {
                noMargin,
                name, br,
                connectionType, br,
                launchCommand, br,
                launchArguments, br,
                envChanges, br,
                url, br,
                httpHeaders, br,
            };
            // clang-format on
        });
    }

    McpManager::ServerInfo toServerInfo() const
    {
        McpManager::ServerInfo info;
        info.id = id.value();
        info.name = name.value();
        info.connectionType = McpManager::ConnectionType(
            conTypeEnum().keyToValue(connectionType.value().toUtf8()));

        if (info.connectionType == McpManager::ConnectionType::Stdio) {
            QString args = launchArguments.value();

            // Replace {env-vars} placeholder with -e KEY=VALUE args from env changes.
            // With docker, omitting = unsets the variable in the container, so we don't
            // need to pass env changes through to the process environment.
            if (args.contains(QLatin1String("{env-vars}"))) {
                QStringList envArgs;
                for (const EnvironmentItem &item : envChanges().itemsFromUser()) {
                    if (item.operation == EnvironmentItem::SetEnabled)
                        envArgs.append(QString("-e %1=%2").arg(item.name, item.value));
                    else if (item.operation == EnvironmentItem::Unset)
                        envArgs.append(QString("-e %1").arg(item.name));
                }
                args.replace(QLatin1String("{env-vars}"), envArgs.join(' '));
            } else {
                info.envChanges = envChanges();
            }

            info.launchInfo = Utils::CommandLine(
                FilePath::fromUserInput(launchCommand.value()), args, Utils::CommandLine::Raw);
        } else {
            info.launchInfo = QUrl(url.value());
        }
        info.httpHeaders = httpHeaders();
        return info;
    }

    StringAspect id{this};
    StringAspect name{this};
    StringSelectionAspect connectionType{this};

    FilePathAspect launchCommand{this};
    StringAspect launchArguments{this};
    EnvironmentChangesAspect envChanges{this};

    StringAspect url{this};
    StringListAspect httpHeaders{this};
};

static QVariant displayFunc(McpServerAspect *aspect, int role)
{
    if (role == Qt::DisplayRole)
        return aspect->name.volatileValue();
    return {};
}

static std::optional<QString> toString(const McpRegistry::Argument &arg)
{
    if (arg.value().has_value())
        return arg.value();
    if (arg.default_().has_value())
        return arg.default_();
    if (arg.value_hint().has_value())
        return arg.value_hint();
    return std::nullopt;
}

static QStringList serializeArguments(const QList<McpRegistry::Argument> &arguments)
{
    QStringList result;
    for (const McpRegistry::Argument &arg : arguments) {
        const std::optional<QString> argValue = toString(arg);
        if (arg.type() == McpRegistry::Argument::Type::named) {
            if (arg.name().has_value()) {
                if (argValue.has_value())
                    result.append(*arg.name() + "=" + *argValue);
                else
                    result.append(*arg.name());
            }
        } else {
            if (argValue.has_value())
                result.append(*argValue);
        }
    }
    return result;
}

static void applyPackageToAspect(McpServerAspect &aspect, const McpRegistry::Package &pkg)
{
    static const auto &conTypeEnum = QMetaEnum::fromType<McpManager::ConnectionType>();

    if (pkg.transport_type() == McpRegistry::Package::Transport_type::stdio) {
        aspect.connectionType.setValue(
            QString::fromLatin1(conTypeEnum.valueToKey(McpManager::Stdio)));

        QString command;
        if (pkg.runtime_hint().has_value()) {
            command = *pkg.runtime_hint();
        } else {
            switch (pkg.registry_type()) {
            case McpRegistry::Package::Registry_type::npm:
                command = QString("npx");
                break;
            case McpRegistry::Package::Registry_type::pypi:
                command = QString("uvx");
                break;
            case McpRegistry::Package::Registry_type::oci:
                command = QString("docker");
                break;
            case McpRegistry::Package::Registry_type::nuget:
                command = QString("dnx");
                break;
            case McpRegistry::Package::Registry_type::mcpb:
                command = QString("mcpb");
                break;
            }
        }

        aspect.launchCommand.setValue(FilePath::fromUserInput(command));

        const QStringList runtimeArgs = serializeArguments(
            pkg.runtime_arguments().value_or(QList<McpRegistry::Argument>{}));
        const QStringList packageArgs = serializeArguments(
            pkg.package_arguments().value_or(QList<McpRegistry::Argument>{}));

        QStringList argParts;

        if (pkg.registry_type() == McpRegistry::Package::Registry_type::oci) {
            argParts.append("container run --rm -i {env-vars}");
            argParts.append(runtimeArgs);
            QString image = pkg.identifier();
            if (pkg.version().has_value())
                image += QString(":") + *pkg.version();
            argParts.append(image);
            argParts.append(packageArgs);
        } else {
            argParts.append(runtimeArgs);
            QString pkgId = pkg.identifier();
            if (pkg.version().has_value())
                pkgId += QString("@") + *pkg.version();
            argParts.append(pkgId);
            argParts.append(packageArgs);
        }

        aspect.launchArguments.setValue(argParts.join(' '));
    } else {
        // streamable-http or sse transport via package
        if (pkg.transport_type() == McpRegistry::Package::Transport_type::sse) {
            aspect.connectionType.setValue(
                QString::fromLatin1(conTypeEnum.valueToKey(McpManager::Sse)));
        } else {
            aspect.connectionType.setValue(
                QString::fromLatin1(conTypeEnum.valueToKey(McpManager::Streamable_Http)));
        }
    }

    // Prefill environment variables from registry
    if (pkg.env_vars().has_value() && !pkg.env_vars()->isEmpty()) {
        EnvironmentItems items;
        for (const auto &envVar : *pkg.env_vars()) {
            if (envVar.description().has_value() && !envVar.description()->isEmpty()) {
                items.append(
                    EnvironmentItem(" " + *envVar.description(), QString(), EnvironmentItem::Comment));
            }
            const QString value = envVar.default_().value_or(QString());
            const auto op = envVar.required() ? EnvironmentItem::SetEnabled
                                              : EnvironmentItem::Comment;
            items.append(EnvironmentItem(envVar.name(), value, op));
        }
        EnvironmentChanges changes;
        changes.setItemsFromUser(items);
        aspect.envChanges.setValue(changes);
    }

    // Prefill HTTP headers from registry (for streamable-http / sse transports)
    if (pkg.headers().has_value()) {
        QStringList headerList;
        for (const auto &h : *pkg.headers())
            headerList.append(h.name() + QLatin1String(": ") + h.default_().value_or(QString()));
        aspect.httpHeaders.setValue(headerList);
    }
}

static void applyRemoteToAspect(McpServerAspect &aspect, const McpRegistry::Remote &remote)
{
    static const auto &conTypeEnum = QMetaEnum::fromType<McpManager::ConnectionType>();
    if (remote.type() == McpRegistry::Remote::Type::sse) {
        aspect.connectionType.setValue(QString::fromLatin1(conTypeEnum.valueToKey(McpManager::Sse)));
    } else {
        aspect.connectionType.setValue(
            QString::fromLatin1(conTypeEnum.valueToKey(McpManager::Streamable_Http)));
    }
    aspect.url.setValue(remote.url());

    if (remote.headers().has_value()) {
        QStringList headerList;
        for (const auto &h : *remote.headers())
            headerList.append(
                h.name() + QLatin1String(": ")
                + h.default_().value_or(h.description().value_or(QString())));
        aspect.httpHeaders.setValue(headerList);
    }
}

struct ConnectionOption
{
    QString label;
    int packageIndex = -1; // index into server.packages(), or -1
    int remoteIndex = -1;  // index into server.remotes(), or -1
};

static QList<ConnectionOption> connectionOptions(const McpRegistry::Server &server)
{
    QList<ConnectionOption> options;
    int i = 0;
    for (const auto &pkg : server.packages().value_or(QList<McpRegistry::Package>{})) {
        QString label = pkg.identifier();
        if (pkg.version().has_value())
            label += QString("@") + *pkg.version();
        label += QString(" (%1, %2)")
                     .arg(
                         McpRegistry::toString(pkg.registry_type()),
                         McpRegistry::toString(pkg.transport_type()));
        options.append({label, i, -1});
        ++i;
    }
    i = 0;
    for (const auto &remote : server.remotes().value_or(QList<McpRegistry::Remote>{})) {
        QString label = remote.url() + QString(" (%1)").arg(McpRegistry::toString(remote.type()));
        options.append({label, -1, i});
        ++i;
    }
    return options;
}

static int chooseConnectionOption(const QList<ConnectionOption> &options, QWidget *parent)
{
    if (options.size() <= 1)
        return 0;

    QDialog dlg(parent);
    dlg.setWindowTitle(Tr::tr("Choose Connection Type"));

    auto *layout = new QVBoxLayout(&dlg);
    layout->addWidget(new QLabel(
        Tr::tr(
            "This server offers multiple ways to connect. "
            "Please choose one:"),
        &dlg));

    auto *listView = new QListView(&dlg);
    auto *model = new QStringListModel(&dlg);
    QStringList labels;
    for (const auto &opt : options)
        labels.append(opt.label);
    model->setStringList(labels);
    listView->setModel(model);
    listView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    listView->setCurrentIndex(model->index(0, 0));
    layout->addWidget(listView);

    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    layout->addWidget(buttonBox);

    QObject::connect(listView, &QListView::doubleClicked, &dlg, &QDialog::accept);
    QObject::connect(buttonBox, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    QObject::connect(buttonBox, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    if (dlg.exec() != QDialog::Accepted)
        return -1;

    return listView->currentIndex().row();
}

static std::shared_ptr<McpServerAspect> createAspectFromRegistryServer(
    const McpRegistry::Server &server, QWidget *parent)
{
    const auto options = connectionOptions(server);
    if (options.isEmpty())
        return {};

    int chosen = 0;
    if (options.size() > 1) {
        chosen = chooseConnectionOption(options, parent);
        if (chosen < 0)
            return {};
    }

    auto aspect = std::make_shared<McpServerAspect>();
    const QString displayName = server.title().value_or(server.name());
    aspect->name.setValue(displayName);

    const auto &opt = options.at(chosen);
    if (opt.packageIndex >= 0)
        applyPackageToAspect(*aspect, server.packages()->at(opt.packageIndex));
    else if (opt.remoteIndex >= 0)
        applyRemoteToAspect(*aspect, server.remotes()->at(opt.remoteIndex));

    return aspect;
}

class McpManagerSettings : public Utils::AspectContainer
{
public:
    McpManagerSettings()
    {
        setAutoApply(false);
        mcpServers.setSettingsKey("McpServers");
        mcpServers.setDisplayStyle(AspectList::DisplayStyle::ListViewWithDetails);
        mcpServers.setCreateItemFunction([] {
            auto newItem = std::make_shared<McpServerAspect>();
            return newItem;
        });

        mcpServers.listViewDataCallback = displayFunc;
        mcpServers.addExtraButton(Tr::tr("Add From Registry..."), [this] { addFromRegistry(); });

        enableDocsMcpServer.setLabelText(Tr::tr("Enable Qt Documentation MCP Server"));
        enableDocsMcpServer.setToolTip(
            Tr::tr(
                "When enabled, the Qt documentation MCP server from https://qt-docs-mcp.qt.io/mcp "
                "will be used."));
        enableDocsMcpServer.setDefaultValue(true);
        enableDocsMcpServer.setSettingsKey("EnableQtDocsServer");

        connect(&enableDocsMcpServer, &BoolAspect::changed, this, [this] {
            if (enableDocsMcpServer())
                registerQtDocsServer();
            else
                removeQtDocsServer();
        });

        setLayouter([this]() {
            using namespace Layouting;
            // clang-format off
            return Column{
                enableDocsMcpServer,
                &mcpServers,
            };
            // clang-format on
        });

        readSettings();
        if (enableDocsMcpServer())
            QTimer::singleShot(0, this, &McpManagerSettings::registerQtDocsServer);
    }

    static McpManagerSettings &instance()
    {
        static GuardedObject<McpManagerSettings> settings;
        return settings;
    }

    void registerQtDocsServer()
    {
        const auto serverInfo = McpManager::ServerInfo{
            QT_DOCS_MCPSERVER_MANAGER_ID,
            "Qt Documentation builtin server",
            McpManager::ConnectionType::Streamable_Http,
            QUrl("https://qt-docs-mcp.qt.io/mcp"),
            {},
            {}};
        QTC_CHECK(McpManager::registerMcpServer(serverInfo));
    }

    void addFromRegistry()
    {
        McpRegistryDialog dlg(ICore::dialogParent());
        if (dlg.exec() != QDialog::Accepted)
            return;
        const auto server = dlg.selectedServer();
        if (!server)
            return;
        auto aspect = createAspectFromRegistryServer(*server, ICore::dialogParent());
        if (!aspect)
            return;
        mcpServers.addItem(aspect);
    }

    void removeQtDocsServer() { McpManager::removeMcpServer(QT_DOCS_MCPSERVER_MANAGER_ID); }

    AspectList mcpServers{this};
    BoolAspect enableDocsMcpServer{this};
};

class McpManagerSettingsPage final : public IOptionsPage
{
public:
    McpManagerSettingsPage()
    {
        setId(MCPMANAGER_SETTINGS_PAGE_ID);
        setDisplayName(Tr::tr("MCP Servers"));
        setCategory(Core::Constants::SETTINGS_CATEGORY_AI);
        setSettingsProvider([] { return &McpManagerSettings::instance(); });
    }
};

static McpManagerSettingsPage &settingsPage()
{
    static McpManagerSettingsPage page;
    return page;
}

static QList<McpManager::ServerInfo> &builtInMcpServers()
{
    static QList<McpManager::ServerInfo> servers;
    return servers;
}

McpManager::McpManager()
{
    QObject::connect(
        &McpManagerSettings::instance().mcpServers,
        &BaseAspect::changed,
        this,
        &McpManager::mcpServersChanged);
}

McpManager::~McpManager() = default;

McpManager &McpManager::instance()
{
    static McpManager manager;
    return manager;
}

bool McpManager::registerMcpServer(McpManager::ServerInfo info)
{
    if (Utils::anyOf(mcpServers(), [&info](const McpManager::ServerInfo &s) {
            return s.id == info.id;
        }))
        return false;

    builtInMcpServers().append(info);
    emit instance().mcpServersChanged();
    return true;
}

void McpManager::removeMcpServer(const QString &id)
{
    auto &servers = builtInMcpServers();
    auto it = std::remove_if(servers.begin(), servers.end(), [&id](const McpManager::ServerInfo &s) {
        return s.id == id;
    });
    if (it != servers.end()) {
        servers.erase(it, servers.end());
        emit instance().mcpServersChanged();
    }
}

QList<McpManager::ServerInfo> McpManager::mcpServers()
{
    QList<McpManager::ServerInfo> servers = builtInMcpServers();

    McpManagerSettings::instance().mcpServers.forEachItem([&](McpServerAspect *server) {
        McpManager::ServerInfo info = server->toServerInfo();
        servers.push_back(info);
    });

    return servers;
}

void setupMcpManager()
{
    (void) settingsPage();
    (void) McpManager::instance();
}

} // namespace Core

#include "mcpmanager.moc"
