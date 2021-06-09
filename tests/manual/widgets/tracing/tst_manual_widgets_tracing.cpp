/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include <QApplication>
#include <QQuickView>
#include <QQmlContext>

#include <tracing/timelinerenderer.h>
#include <tracing/timelineoverviewrenderer.h>
#include <tracing/timelinemodelaggregator.h>
#include <tracing/timelinetheme.h>
#include <tracing/timelineformattime.h>
#include <tracing/timelinezoomcontrol.h>

#include "../common/themeselector.h"

using namespace Timeline;

static const qint64 oneMs = 1000 * 1000; // in nanoseconds

class DummyModel : public Timeline::TimelineModel
{
public:
    DummyModel(TimelineModelAggregator *parent)
        : TimelineModel(parent)
    {
        setDisplayName("Dummy Category");
        setCollapsedRowCount(1);
        setExpandedRowCount(3);
        setTooltip("This is a tool tip"); // Tooltip on the title on the left
        setCategoryColor(Qt::yellow);
    }

    QRgb color(int index) const override
    {
        return QColor::fromHsl(index * 20, 96, 128).rgb(); // Red to green
    }

    // Called when a range gets selected, requests index of row which a range belongs to
    int expandedRow(int index) const override
    {
        return selectionId(index) % expandedRowCount(); // Distribute selections among rows
    }

    // Called when the category gets expanded
    QVariantList labels() const override
    {
        QVariantList result;

        QVariantMap element;
        element.insert(QLatin1String("description"), QVariant("Dummy sub category 1"));
        element.insert(QLatin1String("id"), 0);
        result << element;

        element.clear();
        element.insert(QLatin1String("description"), QVariant("Dummy sub category 2"));
        element.insert(QLatin1String("id"), 1);
        result << element;

        return result;
    }

    // Called when a range (or track) is selected
    QVariantMap details(int index) const override
    {
        QVariantMap result;
        result.insert(QLatin1String("displayName"),
                      QString::fromLatin1("Details for range %1").arg(index));
        result.insert(QLatin1String("Foo A"), QString::fromLatin1("Bar %1a").arg(index));
        result.insert(QLatin1String("Foo B"), QString::fromLatin1("Bar %1b").arg(index));
        return result;
    }

    float relativeHeight(int index) const override
    {
        return 1.0 - (index / 10.0); // Shrink over time
    }

    void populateData()
    {
        // One region per selection
        insert(oneMs *  20, oneMs * 20, 0);
        insert(oneMs * 100, oneMs * 30, 1);

        // Two regions in the same selection
        insert(oneMs *  40, oneMs * 20, 2);
        insert(oneMs * 120, oneMs * 30, 2);

        // Three regions in the same selection
        insert(oneMs *  60, oneMs * 30, 3);
        insert(oneMs * 140, oneMs * 20, 3);
        insert(oneMs * 170, oneMs * 20, 3);

        emit contentChanged();
    }
};

class TraceView : public QQuickView
{
public:
    TraceView(QWindow *parent = 0)
        : QQuickView(parent)
    {
        setResizeMode(QQuickView::SizeRootObjectToView);

#if QT_VERSION < QT_VERSION_CHECK(6, 2, 0)
        qmlRegisterType<TimelineRenderer>("QtCreator.Tracing", 1, 0, "TimelineRenderer");
        qmlRegisterType<TimelineOverviewRenderer>(
                    "QtCreator.Tracing", 1, 0, "TimelineOverviewRenderer");
        qmlRegisterAnonymousType<TimelineZoomControl>("QtCreator.Tracing", 1);
        qmlRegisterAnonymousType<TimelineModel>("QtCreator.Tracing", 1);
        qmlRegisterAnonymousType<TimelineNotesModel>("QtCreator.Tracing", 1);
#endif // Qt < 6.2

        TimelineTheme::setupTheme(engine());
        TimeFormatter::setupTimeFormatter();

        m_modelAggregator = new TimelineModelAggregator(this);
        m_model = new DummyModel(m_modelAggregator);
        m_model->populateData();
        m_modelAggregator->setModels({QVariant::fromValue(m_model)});
        rootContext()->setContextProperty(QLatin1String("timelineModelAggregator"),
                                          m_modelAggregator);

        m_zoomControl = new TimelineZoomControl(this);
        m_zoomControl->setTrace(0, oneMs * 1000); // Total timeline length
        rootContext()->setContextProperty("zoomControl", m_zoomControl);
        setSource(QUrl(QLatin1String("qrc:/QtCreator/Tracing/MainView.qml")));

        // Zoom onto first timeline third. Needs to be done after loading setSource.
        m_zoomControl->setRange(0, oneMs * 1000 / 3.0);
    }

   ~TraceView() override = default;

    DummyModel *m_model;
    TimelineModelAggregator *m_modelAggregator;
    TimelineZoomControl *m_zoomControl;
};

int main(int argc, char *argv[])
{
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

    QApplication app(argc, argv);

    ManualTest::ThemeSelector::setTheme(":/themes/flat.creatortheme");

    auto view = new TraceView;
    view->resize(700, 300);
    view->show();

    return app.exec();
}
