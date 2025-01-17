// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qtacademywelcomepage.h"

#include "learningtr.h"

#include <coreplugin/welcomepagehelper.h>

#include <solutions/spinner/spinner.h>
#include <solutions/tasking/networkquery.h>
#include <solutions/tasking/tasktree.h>
#include <solutions/tasking/tasktreerunner.h>

#include <utils/fileutils.h>
#include <utils/layoutbuilder.h>
#include <utils/networkaccessmanager.h>

#include <QApplication>
#include <QDesktopServices>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLoggingCategory>
#include <QNetworkReply>
#include <QPixmapCache>

using namespace Utils;
using namespace Core;

Q_LOGGING_CATEGORY(qtAcademyLog, "qtc.qtacademy", QtWarningMsg)

namespace Learning::Internal {

class CourseItem : public ListItem
{
public:
    QString id;
};

class CourseItemDelegate : public ListItemDelegate
{
public:
    void clickAction(const ListItem *item) const override
    {
        QTC_ASSERT(item, return);
        auto courseItem = static_cast<const CourseItem *>(item);
        const QUrl url(QString("https://academy.qt.io/catalog/courses/").append(courseItem->id));
        QDesktopServices::openUrl(url);
    }
};

static QString courseName(const QJsonObject &courseObj)
{
    return courseObj.value("name").toString().trimmed();
}

static QString courseDescription(const QJsonObject &courseObj)
{
    QString rawText = courseObj.value("objectives_text").toString();
    const qsizetype maxTextLength = 200;
    const QString elide = " ...";
    if (rawText.size() > maxTextLength - elide.size())
        rawText = rawText.left(maxTextLength) + elide;
    const QStringList rawLines = rawText.split("\r\n");
    QStringList lines;
    for (bool prevLineEmpty = false; const QString &rawLine : rawLines) {
        const QString line = rawLine.trimmed();
        if (prevLineEmpty && line.isEmpty())
            continue;
        prevLineEmpty = line.isEmpty();
        lines.append(line);
    }
    return lines.join("\n");
}

static QString courseThumbnail(const QJsonObject &courseObj)
{
    return courseObj.value("thumbnail_image_url").toString();
}

static QStringList courseTags(const QJsonObject &courseObj)
{
    return courseObj.value("keywords").toString().split(", ");
}

static QString courseId(const QJsonObject &courseObj)
{
    return QString::number(courseObj.value("id").toInt());
}

static void setJson(const QByteArray &json, ListModel *model)
{
    QJsonParseError error;
    const QJsonObject jsonObj = QJsonDocument::fromJson(json, &error).object();
    qCDebug(qtAcademyLog) << "QJsonParseError:" << error.errorString();
    const QJsonArray courses = jsonObj.value("courses").toArray();
    QList<ListItem *> items;
    for (const auto course : courses) {
        auto courseItem = new CourseItem;
        const QJsonObject courseObj = course.toObject();
        courseItem->name = courseName(courseObj);
        courseItem->description = courseDescription(courseObj);
        courseItem->imageUrl = courseThumbnail(courseObj);
        courseItem->tags = courseTags(courseObj);
        courseItem->id = courseId(courseObj);
        items.append(courseItem);
    }
    model->appendItems(items);
}

class QtAcademyWelcomePageWidget final : public QWidget
{
public:
    QtAcademyWelcomePageWidget()
    {
        m_searcher = new SearchBox(this);
        m_searcher->setPlaceholderText(Tr::tr("Search in Qt Academy Courses..."));

        m_model = new ListModel;
        m_model->setPixmapFunction([this](const QString &url) -> QPixmap {
            queueImageForDownload(url);
            return {};
        });

        m_filteredModel = new ListModelFilter(m_model, this);

        m_view = new GridView;
        m_view->setModel(m_filteredModel);
        m_view->setItemDelegate(&m_delegate);

        using namespace StyleHelper::SpacingTokens;
        using namespace Layouting;
        Column {
            Row {
                m_searcher,
                customMargins(0, 0, ExVPaddingGapXl, 0),
            },
            m_view,
            spacing(ExVPaddingGapXl),
            customMargins(ExVPaddingGapXl, ExVPaddingGapXl, 0, 0),
        }.attachTo(this);

        connect(m_searcher, &QLineEdit::textChanged,
                m_filteredModel, &ListModelFilter::setSearchString);
        connect(&m_delegate, &CourseItemDelegate::tagClicked,
                this, &QtAcademyWelcomePageWidget::onTagClicked);

        m_spinner = new SpinnerSolution::Spinner(SpinnerSolution::SpinnerSize::Large, this);
        m_spinner->hide();
    };

    void showEvent(QShowEvent *event) override
    {
        if (!m_dataFetched) {
            m_dataFetched = true;
            fetchExtensions();
        }
        QWidget::showEvent(event);
    }

private:
    void fetchExtensions()
    {
#ifdef WITH_TESTS
        // Uncomment for testing with local json data.
        // setJson(FileReader::fetchQrc(":/learning/testdata/courses.json"), m_model); return;
#endif // WITH_TESTS

        using namespace Tasking;

        const auto onQuerySetup = [this](NetworkQuery &query) {
            const QString request = "https://www.qt.io/hubfs/Academy/courses.json";
            query.setRequest(QNetworkRequest(QUrl::fromUserInput(request)));
            query.setNetworkAccessManager(NetworkAccessManager::instance());
            qCDebug(qtAcademyLog).noquote() << "Sending JSON request:" << request;
            m_spinner->show();
        };
        const auto onQueryDone = [this](const NetworkQuery &query, DoneWith result) {
            const QByteArray response = query.reply()->readAll();
            qCDebug(qtAcademyLog).noquote() << "Got JSON QNetworkReply:" << query.reply()->error();
            if (result == DoneWith::Success) {
                qCDebug(qtAcademyLog).noquote() << "JSON response size:"
                                                << QLocale::system().formattedDataSize(response.size());
                setJson(response, m_model);
            } else {
                qCWarning(qtAcademyLog).noquote() << response;
                setJson({}, m_model);
            }
            m_spinner->hide();
        };

        Group group {
            NetworkQueryTask{onQuerySetup, onQueryDone},
        };

        taskTreeRunner.start(group);
    }

    void queueImageForDownload(const QString &url)
    {
        m_pendingImages.insert(url);
        if (!m_isDownloadingImage)
            fetchNextImage();
    }

    void updateModelIndexesForUrl(const QString &url)
    {
        const QList<ListItem *> items = m_model->items();
        for (int row = 0, end = items.size(); row < end; ++row) {
            if (items.at(row)->imageUrl == url) {
                const QModelIndex index = m_model->index(row);
                emit m_model->dataChanged(index, index, {ListModel::ItemImageRole,
                                                         Qt::DisplayRole});
            }
        }
    }

    void fetchNextImage()
    {
        if (m_pendingImages.isEmpty()) {
            m_isDownloadingImage = false;
            return;
        }

        const auto it = m_pendingImages.constBegin();
        const QString nextUrl = *it;
        m_pendingImages.erase(it);

        if (QPixmapCache::find(nextUrl, nullptr)) {
            // this image is already cached it might have been added while downloading
            updateModelIndexesForUrl(nextUrl);
            fetchNextImage();
            return;
        }

        m_isDownloadingImage = true;
        QNetworkReply *reply = Utils::NetworkAccessManager::instance()->get(QNetworkRequest(nextUrl));
        connect(reply, &QNetworkReply::finished,
                this, [this, reply]() { onImageDownloadFinished(reply); });
    }

    void onImageDownloadFinished(QNetworkReply *reply)
    {
        QTC_ASSERT(reply, return);
        const QScopeGuard cleanup([reply] { reply->deleteLater(); });

        if (reply->error() == QNetworkReply::NoError) {
            const QByteArray data = reply->readAll();
            QPixmap pixmap;
            const QUrl imageUrl = reply->request().url();
            const QString imageFormat = QFileInfo(imageUrl.fileName()).suffix();
            if (pixmap.loadFromData(data, imageFormat.toLatin1())) {
                const QString url = imageUrl.toString();
                const int dpr = qApp->devicePixelRatio();
                pixmap = pixmap.scaled(WelcomePageHelpers::WelcomeThumbnailSize * dpr,
                                       Qt::KeepAspectRatio,
                                       Qt::SmoothTransformation);
                pixmap.setDevicePixelRatio(dpr);
                QPixmapCache::insert(url, pixmap);
                updateModelIndexesForUrl(url);
            }
        } // handle error not needed - it's okay'ish to have no images as long as the rest works

        fetchNextImage();
    }

    void onTagClicked(const QString &tag)
    {
        const QString tagStr("tag:");
        const QString text = m_searcher->text();
        m_searcher->setText((text.startsWith(tagStr + "\"") ? text.trimmed() + " " : QString())
                            + QString(tagStr + "\"%1\" ").arg(tag));
    }

    QLineEdit *m_searcher;
    ListModel *m_model;
    ListModelFilter *m_filteredModel;
    GridView *m_view;
    CourseItemDelegate m_delegate;
    bool m_dataFetched = false;
    QSet<QString> m_pendingImages;
    bool m_isDownloadingImage = false;
    Tasking::TaskTreeRunner taskTreeRunner;
    SpinnerSolution::Spinner *m_spinner;
};

class QtAcademyWelcomePage final : public IWelcomePage
{
public:
    QtAcademyWelcomePage() = default;

    QString title() const final { return Tr::tr("Qt Academy"); }
    int priority() const final { return 60; }
    Utils::Id id() const final { return "QtAcademy"; }
    QWidget *createWidget() const final { return new QtAcademyWelcomePageWidget; }
};

void setupQtAcademyWelcomePage(QObject *guard)
{
    auto page = new QtAcademyWelcomePage;
    page->setParent(guard);
}

} // namespace Learning::Internal
