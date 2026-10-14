// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "perfsamplersettings_test.h"

#include <profiler/combinedsampler.h>
#include <profiler/perfsampler.h>

#include <utils/layoutbuilder.h>

#include <QAbstractButton>
#include <QComboBox>
#include <QTableView>
#include <QTest>

namespace Profiler::Internal {

class PerfSamplerSettingsTest final : public QObject
{
    Q_OBJECT

private slots:
    void testOptionsWidget_data()
    {
        QTest::addColumn<bool>("combined");
        QTest::newRow("standalone") << false;
        QTest::newRow("combined") << true;
    }

    void testOptionsWidget()
    {
        QFETCH(bool, combined);
        PerfSamplerSettings perfSettings;
        perfSettings.perfSettings.callgraphMode.setValue(0);
        perfSettings.downloadDebugInfo.setValue(false);
        CombinedSamplerSettings combinedSettings;
        combinedSettings.setPerfSamplerSettings(&perfSettings);
        SamplerSettings *settings = combined ? static_cast<SamplerSettings *>(&combinedSettings)
                                            : &perfSettings;
        QWidget widget;
        settings->layouter()().attachTo(&widget);

        QTableView *events = widget.findChild<QTableView *>();
        QVERIFY(events);
        QVERIFY(events->isVisibleTo(&widget));

        QComboBox *callgraph = nullptr;
        for (QComboBox *combo : widget.findChildren<QComboBox *>()) {
            if (combo->findText("frame pointer") >= 0)
                callgraph = combo;
        }
        QVERIFY(callgraph);
        QVERIFY(callgraph->isVisibleTo(&widget));
        callgraph->setCurrentIndex(callgraph->findText("frame pointer"));
        QCOMPARE(perfSettings.perfSettings.callgraphMode.itemValue().toString(), "fp");
        QVERIFY(perfSettings.perfSettings.perfRecordArguments().contains("--call-graph fp"));

        QAbstractButton *download = nullptr;
        for (QAbstractButton *button : widget.findChildren<QAbstractButton *>()) {
            if (button->text() == "Download missing debug information")
                download = button;
        }
        QVERIFY(download);
        QVERIFY(download->isVisibleTo(&widget));
        download->click();
        QVERIFY(perfSettings.downloadDebugInfo());

        settings->setOptionsChosenElsewhere(true);
        QVERIFY(!callgraph->isEnabled());
        QVERIFY(!download->isEnabled());
        settings->setOptionsChosenElsewhere(false);
        QVERIFY(callgraph->isEnabled());
        QVERIFY(download->isEnabled());
    }

    // Settings load quietly, without the signal a changed selection emits.
    void testStackSizeFollowsLoadedCallgraphMode()
    {
        PerfSamplerSettings samplerSettings;
        PerfSettings &settings = samplerSettings.perfSettings;
        settings.callgraphMode.setValue(1, Utils::BaseAspect::BeQuiet); // "fp"
        QWidget fpWidget;
        settings.layouter()().attachTo(&fpWidget);
        QVERIFY(!settings.stackSize.isEnabled());

        settings.callgraphMode.setValue(0, Utils::BaseAspect::BeQuiet); // "dwarf"
        QWidget dwarfWidget;
        settings.layouter()().attachTo(&dwarfWidget);
        QVERIFY(settings.stackSize.isEnabled());

        Utils::Store map;
        settings.toMap(map);
        map.insert(settings.callgraphMode.settingsKey(), 1);
        settings.fromMap(map);
        QVERIFY(!settings.stackSize.isEnabled());
    }
};

QObject *createPerfSamplerSettingsTest()
{
    return new PerfSamplerSettingsTest;
}

} // namespace Profiler::Internal

#include "perfsamplersettings_test.moc"
