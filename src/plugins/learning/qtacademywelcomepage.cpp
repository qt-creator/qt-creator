// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qtacademywelcomepage.h"

#include "learningtr.h"
#include "utils/algorithm.h"

#include <coreplugin/welcomepagehelper.h>

#include <solutions/spinner/spinner.h>
#include <solutions/tasking/networkquery.h>
#include <solutions/tasking/tasktree.h>
#include <solutions/tasking/tasktreerunner.h>

#include <utils/fileutils.h>
#include <utils/layoutbuilder.h>
#include <utils/networkaccessmanager.h>
#include <utils/qtcwidgets.h>
#include <utils/utilsicons.h>

#include <QApplication>
#include <QDesktopServices>
#include <QGridLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLoggingCategory>
#include <QMouseEvent>
#include <QNetworkReply>
#include <QPainter>
#include <QPainterPath>
#include <QPixmapCache>
#include <QScrollArea>

using namespace Utils;
using namespace Core;

Q_LOGGING_CATEGORY(qtAcademyLog, "qtc.qtacademy", QtWarningMsg)

namespace Learning::Internal {

struct Reviews
{
    int numReviews{0};
    int numStars{0};
    float value{0.0f};
};
class CourseItem : public ListItem
{
public:
    bool isPath{false};
    QString id;
    QString rawName;
    std::optional<Reviews> reviews;
    QString difficultyLevel;
    QString duration;
    std::optional<QString> objectivesHtml;
    QString descriptionHtml;
};

static QString courseUrl(const CourseItem *item)
{
    if (item->isPath)
        return QString("https://academy.qt.io/catalog/learning-paths/%1").arg(item->id);
    else
        return QString("https://academy.qt.io/catalog/courses/%1").arg(item->id);
}

class CourseItemDelegate : public ListItemDelegate
{
    Q_OBJECT
public:
    void clickAction(const ListItem *item) const override { emit clicked(item); }

signals:
    void clicked(const ListItem *item) const;
};

static QString courseName(const QJsonObject &courseObj)
{
    return courseObj.value("name").toString();
}

static QString courseDescription(const QJsonObject &courseObj)
{
    QString rawText = courseObj.value("objectives_text").toString();
    const qsizetype maxTextLength = 200;
    const QString elide = " ...";
    if (rawText.size() > maxTextLength - elide.size())
        rawText = rawText.left(maxTextLength) + elide;
    const QStringList rawLines = rawText.split("\n");
    QStringList lines;
    for (bool prevLineEmpty = false; const QString &rawLine : rawLines) {
        const QString line = rawLine.trimmed();
        if (prevLineEmpty && line.isEmpty())
            continue;
        prevLineEmpty = line.isEmpty();
        lines.append(line);
    }
    return lines.join("\n").replace("&nbsp;", QChar(QChar::Nbsp));
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

static std::optional<Reviews> courseReviews(const QJsonObject &courseObj)
{
    if (courseObj.contains("number_of_reviews") && courseObj.contains("number_of_stars")) {
        Reviews reviews;
        reviews.numReviews = courseObj.value("number_of_reviews").toInt(0);
        reviews.numStars = courseObj.value("number_of_stars").toInt(0);
        reviews.value = ((float) reviews.numStars / (float) reviews.numReviews) / 5.0f;
        return reviews;
    }
    return std::nullopt;
}

static QString courseDifficultyLevel(const QJsonObject &courseObj)
{
    return courseObj.value("difficulty_level").toString();
}

static QString courseDuration(const QJsonObject &courseObj)
{
    int length = courseObj.value("minute_length").toInt();
    QString units;
    if (courseObj.contains("course_length_unit"))
        units = courseObj.value("course_length_unit").toString();
    else if (courseObj.contains("path_length_unit"))
        units = courseObj.value("path_length_unit").toString();

    if (units == "hours")
        length *= 60;
    length *= 60 * 1000;

    auto t = QTime::fromMSecsSinceStartOfDay(length);
    if (t.hour() != 0 && t.minute() != 0)
        return QString("%1%2 %3%4")
            .arg(t.hour())
            .arg(Tr::tr("h", "hours"))
            .arg(t.minute())
            .arg(Tr::tr("min", "minutes"));
    if (t.hour() != 0)
        return QString("%1%2").arg(t.hour()).arg(Tr::tr("h", "hours"));

    return QString("%1%2").arg(t.minute()).arg(Tr::tr("min", "minutes"));
}

static std::optional<QString> courseObjectivesHtml(const QJsonObject &courseObj)
{
    if (!courseObj.contains("objectives_html"))
        return std::nullopt;
    return courseObj.value("objectives_html").toString();
}

static QString courseDescriptionHtml(const QJsonObject &courseObj)
{
    return courseObj.value("description_html").toString();
}

static bool courseIsValid(const QJsonObject &courseObj)
{
    const bool cataloged = courseObj.value("cataloged").toBool();
    if (!cataloged)
        return false;

    const QStringList courseKeywords = courseTags(courseObj);
    const bool keywordsValid = Utils::anyOf(courseKeywords, [](const QString &keyword) {
        static const QStringList keywordWhiteList = {
            "Qt Creator",
            "Qt Widgets",
            "Qt for MCUs",
            "Embedded",
            "Developer Tools",
            "Qt Framework",
        };
        return keywordWhiteList.contains(keyword);
    });
    return keywordsValid;
}

static void setJson(const QByteArray &json, ListModel *model)
{
    QJsonParseError error;
    const QJsonObject jsonObj = QJsonDocument::fromJson(json, &error).object();
    qCDebug(qtAcademyLog) << "QJsonParseError:" << error.errorString();
    const QJsonArray courses = jsonObj.value("courses").toArray();
    const QJsonArray learningPaths = jsonObj.value("learningPaths").toArray();

    auto createItemsFromArray = [](const QJsonArray &array, bool isPath = false) {
        QList<ListItem *> items;
        for (const auto course : array) {
            const QJsonObject courseObj = course.toObject();
            if (!courseIsValid(courseObj))
                continue;

            auto courseItem = new CourseItem;
            courseItem->id = courseId(courseObj);
            courseItem->name = courseName(courseObj).trimmed();
            courseItem->rawName = courseName(courseObj);
            courseItem->description = courseDescription(courseObj);
            courseItem->imageUrl = courseThumbnail(courseObj);
            courseItem->tags = courseTags(courseObj);
            courseItem->reviews = courseReviews(courseObj);
            courseItem->difficultyLevel = courseDifficultyLevel(courseObj);
            courseItem->duration = courseDuration(courseObj);
            courseItem->objectivesHtml = courseObjectivesHtml(courseObj);
            courseItem->descriptionHtml = courseDescriptionHtml(courseObj);
            courseItem->isPath = isPath;
            items.append(courseItem);
        }
        return items;
    };

    model->appendItems(createItemsFromArray(courses) + createItemsFromArray(learningPaths, true));
}

static Layouting::Grid createDetailWidget(const CourseItem *course)
{
    static auto blackLabel = [](const QString &text) {
        QLabel *label = new QLabel;
        auto pal = label->palette();
        pal.setColor(QPalette::WindowText, Qt::black);
        label->setPalette(pal);
        label->setText(text);
        return label;
    };

    static auto heading = [](const QString &text) {
        QLabel *label = new QLabel;
        label->setFont(StyleHelper::uiFont(StyleHelper::UiElementH3));
        label->setText(text);
        return label;
    };

    static auto nameLabel = [](const QString &text) {
        QLabel *label = new QLabel;
        label->setFont(StyleHelper::uiFont(StyleHelper::UiElementH2));
        label->setText(text);
        label->setWordWrap(true);
        return label;
    };

    Reviews reviews = course->reviews.value_or(Reviews{});

    auto paintRating = [reviews](QPainter &painter) {
        painter.setRenderHint(QPainter::Antialiasing);

        static QPainterPath starPath = [] {
            QPainterPath p;
            // A 5 sided star with a radius of 10
            p.moveTo(10, 0);
            p.lineTo(12, 7);
            p.lineTo(20, 7);
            p.lineTo(14, 11);
            p.lineTo(16, 18);
            p.lineTo(10, 14);
            p.lineTo(4, 18);
            p.lineTo(6, 11);
            p.lineTo(0, 7);
            p.lineTo(8, 7);
            return p;
        }();

        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor::fromString("#ffc30f"));

        painter.setClipRect(QRectF(0.0, 0.0, 100.0 * reviews.value, 20.0));

        painter.save();
        for (int i = 0; i < 5; i++) {
            painter.drawPath(starPath);
            painter.translate(20, 0);
        }
        painter.restore();
    };

    static auto difficultyLevelTr = [](const CourseItem *course) {
        const auto text = course->difficultyLevel;
        if (text == "basic")
            return Tr::tr("Basic");
        else if (text == "intermediate")
            return Tr::tr("Intermediate");
        else if (text == "advanced")
            return Tr::tr("Advanced");
        else
            return text;
    };

    static auto difficultyColor = [](const CourseItem *course) -> QColor {
        const auto text = course->difficultyLevel;
        if (text == "basic")
            return QColor::fromString("#d2f9d4");
        if (text == "intermediate")
            return QColor::fromString("#f9f2d4");
        if (text == "advanced")
            return QColor::fromString("#f9d4d4");
        return Qt::gray;
    };

    const bool hasObjectives = course->objectivesHtml.has_value();

    using namespace Layouting;

    // clang-format off
    return Grid {
        Align(Qt::AlignLeft | Qt::AlignVCenter, QtcWidgets::Image {
                url(course->imageUrl),
                radius(10),
                minimumWidth(300),
                maximumWidth(300),
            }
        ),
        Column {
            nameLabel(course->name),
            Row {
                QtcWidgets::Rectangle {
                    radius(5),
                    fillBrush(Qt::gray),
                    Row {
                        customMargins(5, 0, 5, 0),
                        Layouting::IconDisplay {
                            icon(Utils::Icons::CLOCK_BLACK)
                        },
                        blackLabel(course->duration)
                    }
                },
                QtcWidgets::Rectangle {
                    radius(5),
                    fillBrush(difficultyColor(course)),
                    Grid {
                        customMargins(5, 0, 5, 0),
                        Align(Qt::AlignCenter, blackLabel(difficultyLevelTr(course))),
                    }
                },
                If(course->reviews.has_value(),
                    {
                        Row {
                            Label {
                                text(QString("%1")
                                    .arg(qFloor((reviews.value*5.0) * 10.0) / 10.0, 0, 'g', 2)),
                            },
                            Canvas {
                                fixedSize(QSize{100, 20}),
                                paint(paintRating),
                            },
                            Label {
                                text(QString("(%1)").arg(reviews.numReviews)),
                            },
                        }
                    }
                ),
                st
            },
            Row {
                QtcWidgets::Button {
                    role(QtcButton::Role::LargePrimary),
                    text(Tr::tr("Start Course")),
                    onClicked(qApp, [course]() {
                        const QUrl url(courseUrl(course));
                        qCDebug(qtAcademyLog) << "QDesktopServices::openUrl" << url;
                        QDesktopServices::openUrl(url);
                    })
                },
                st
            },
            st,
        },
        br,
        Span {
            hasObjectives ? 1 : 2,
            Column {
                heading(Tr::tr("Course Description")),
                Label {
                    wordWrap(true),
                    text(course->descriptionHtml),
                },
                st,
            }
        },
        If (hasObjectives, {
            Column {
                heading(Tr::tr("Objectives")),
                Label {
                    wordWrap(true),
                    text(course->objectivesHtml.value_or(QString{})),
                },
                st
            }
        }),
    };
    // clang-format on
}

class MouseCatcher : public QWidget
{
    Q_OBJECT
public:
    MouseCatcher() { setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding); }

    void mousePressEvent(QMouseEvent *event) override
    {
        if (event->type() == QEvent::MouseButtonPress) {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
            if (mouseEvent->button() == Qt::LeftButton) {
                event->accept();
                emit clicked();
            }
        }
    }

    void paintEvent(QPaintEvent *event) override
    {
        Q_UNUSED(event);
        QPainter painter(this);
        painter.fillRect(rect(), QColor(0, 0, 0, 128));
    }

signals:
    void clicked();
};

class QtAcademyWelcomePageWidget final : public QWidget
{
    Q_OBJECT
public:
    QtAcademyWelcomePageWidget()
    {
        m_searcher = new QtcSearchBox(this);
        m_searcher->setPlaceholderText(Tr::tr("Search for Qt Academy courses..."));

        m_model.setPixmapFunction([this](const QString &url) -> QPixmap {
            queueImageForDownload(url);
            return {};
        });

        m_filteredModel = new ListModelFilter(&m_model, this);

        m_view = new GridView;
        m_view->setModel(m_filteredModel);
        m_view->setItemDelegate(&m_delegate);

        using namespace Layouting;

        // clang-format off
        auto detailWdgt = QtcWidgets::Rectangle {
            Layouting::sizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding)),
            fillBrush(creatorColor(Theme::Color::BackgroundColorNormal)),
            replaceLayoutOn(this, &QtAcademyWelcomePageWidget::courseSelected, [this]() -> Layouting::Layout {
                if (m_selectedCourse) {
                    return Column {
                        ScrollArea {
                            fixSizeHintBug(true),
                            Layouting::sizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding)),
                            frameShape(QFrame::NoFrame),
                            createDetailWidget(m_selectedCourse)
                        }
                    };
                }
                return Row {};
            })
        }.emerge();
        // clang-format on

        auto mouseCatcher = new MouseCatcher;
        connect(mouseCatcher, &MouseCatcher::clicked, this, [this]() {
            setSelectedCourse(nullptr);
        });

        mouseCatcher->setVisible(false);
        detailWdgt->setVisible(false);

        connect(
            this,
            &QtAcademyWelcomePageWidget::courseSelected,
            this,
            [this, detailWdgt, mouseCatcher]() {
                detailWdgt->setVisible(m_selectedCourse);
                mouseCatcher->setVisible(m_selectedCourse);
            });

        using namespace StyleHelper::SpacingTokens;
        using namespace Layouting;
        // clang-format off
        Column {
            Row {
                m_searcher,
                customMargins(0, 0, ExVPaddingGapXl, 0),
            },
            Grid {
                GridCell({
                    m_view,
                    mouseCatcher,
                    Align(Qt::AlignCenter, detailWdgt),
                }),
            },
            spacing(ExVPaddingGapXl),
            customMargins(ExVPaddingGapXl, ExVPaddingGapXl, 0, 0),
        }.attachTo(this);
        // clang-format on

        connect(m_searcher, &QLineEdit::textChanged,
                m_filteredModel, &ListModelFilter::setSearchString);
        connect(&m_delegate, &CourseItemDelegate::tagClicked,
                this, &QtAcademyWelcomePageWidget::onTagClicked);

        connect(&m_delegate, &CourseItemDelegate::clicked, this, [this](const ListItem *item) {
            QTC_ASSERT(item, return);
            setSelectedCourse(static_cast<const CourseItem *>(item));
        });

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
        // setJson(FileUtils::fetchQrc(":/learning/testdata/courses.json").toUtf8(), m_model); return;
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
                setJson(response, &m_model);
            } else {
                qCWarning(qtAcademyLog).noquote() << response;
                setJson({}, &m_model);
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
        const QList<ListItem *> items = m_model.items();
        for (int row = 0, end = items.size(); row < end; ++row) {
            if (items.at(row)->imageUrl == url) {
                const QModelIndex index = m_model.index(row);
                emit m_model.dataChanged(index, index, {ListModel::ItemImageRole, Qt::DisplayRole});
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

    void mousePressEvent(QMouseEvent *event) override
    {
        if (event->button() == Qt::LeftButton)
            setSelectedCourse(nullptr);
    }

    void setSelectedCourse(const CourseItem *course)
    {
        m_selectedCourse = course;
        emit courseSelected();
    }

signals:
    void courseSelected();

private:
    QLineEdit *m_searcher;
    ListModel m_model;
    ListModelFilter *m_filteredModel;
    GridView *m_view;
    CourseItemDelegate m_delegate;
    bool m_dataFetched = false;
    QSet<QString> m_pendingImages;
    bool m_isDownloadingImage = false;
    Tasking::TaskTreeRunner taskTreeRunner;
    SpinnerSolution::Spinner *m_spinner;
    const CourseItem *m_selectedCourse = nullptr;
};

class QtAcademyWelcomePage final : public IWelcomePage
{
public:
    QtAcademyWelcomePage() = default;

    QString title() const final { return Tr::tr("Courses"); }
    int priority() const final { return 60; }
    Utils::Id id() const final { return "Courses"; }
    QWidget *createWidget() const final { return new QtAcademyWelcomePageWidget; }
};

void setupQtAcademyWelcomePage(QObject *guard)
{
    auto page = new QtAcademyWelcomePage;
    page->setParent(guard);
}

} // namespace Learning::Internal

#include "qtacademywelcomepage.moc"
