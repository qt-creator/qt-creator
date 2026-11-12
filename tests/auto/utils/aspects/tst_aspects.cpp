// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <utils/aspects.h>
#include <utils/environment.h>
#include <utils/hostosinfo.h>
#include <utils/infolabel.h>
#include <utils/pathchooser.h>
#include <utils/theme/theme.h>
#include <utils/layoutbuilder.h>

#include <QAbstractButton>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QListWidget>
#include <QSpinBox>
#include <QTemporaryDir>
#include <QTest>
#include <QWidget>

#include <memory>

using namespace Utils;

class tst_Aspects : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void integerAspectUpdatesEverySpinBox();
    void integerAspectTakesEditsFromEverySpinBox();
    void integerAspectKeepsUnscaledValue();
    void doubleAspectUpdatesEverySpinBox();
    void doubleAspectTakesEditsFromEverySpinBox();
    void doubleAspectKeepsUnroundedValue();
    void textDisplayGivesEveryWidgetALabel();
    void textDisplayUpdatesEveryLabel();
    void multiSelectionAspectUpdatesEveryList();
    void multiSelectionAspectTakesEditsFromEveryList();
    void multiSelectionAspectEditKeepsValueOrder();
    void multiSelectionAspectKeepsValueOrder();
    void filePathAspectDefaultValidation();
    void filePathAspectReportsValidity();
    void filePathAspectAddsButtons();
    void filePathAspectUpdatesShownPlaceholder();
    void filePathAspectExpandsVolatilePath();
};

void tst_Aspects::initTestCase()
{
    // Path choosers ask for themed icons, which needs a theme on Linux.
    setCreatorTheme(new Theme("tst_aspects"));
}

void tst_Aspects::cleanupTestCase()
{
    setCreatorTheme(nullptr);
}

static std::unique_ptr<QWidget> createWidget(BaseAspect &aspect)
{
    using namespace Layouting;
    return std::unique_ptr<QWidget>(Column { aspect }.emerge());
}

static QSpinBox *spinBox(const std::unique_ptr<QWidget> &widget)
{
    return widget->findChild<QSpinBox *>();
}

// InfoLabel has no Q_OBJECT; the widget holds nothing but the display's label.
static InfoLabel *infoLabel(const std::unique_ptr<QWidget> &widget)
{
    const QList<QLabel *> labels = widget->findChildren<QLabel *>();
    return labels.size() == 1 ? static_cast<InfoLabel *>(labels.first()) : nullptr;
}

static QListWidget *listWidget(const std::unique_ptr<QWidget> &widget)
{
    return widget->findChild<QListWidget *>();
}

static QStringList checkedItems(QListWidget *list)
{
    QStringList checked;
    for (int i = 0; i < list->count(); ++i) {
        if (list->item(i)->checkState() == Qt::Checked)
            checked.append(list->item(i)->text());
    }
    return checked;
}

static PathChooser *pathChooser(const std::unique_ptr<QWidget> &widget)
{
    return widget->findChild<PathChooser *>();
}

static QStringList buttonTexts(PathChooser *chooser)
{
    QStringList texts;
    for (const QAbstractButton *button : chooser->findChildren<QAbstractButton *>())
        texts.append(button->text());
    return texts;
}

static QDoubleSpinBox *doubleSpinBox(const std::unique_ptr<QWidget> &widget)
{
    return widget->findChild<QDoubleSpinBox *>();
}

void tst_Aspects::integerAspectUpdatesEverySpinBox()
{
    IntegerAspect aspect;
    aspect.setRange(0, 100);
    const auto first = createWidget(aspect);
    const auto second = createWidget(aspect);
    QVERIFY(spinBox(first));
    QVERIFY(spinBox(second));

    aspect.setValue(42);

    QCOMPARE(spinBox(first)->value(), 42);
    QCOMPARE(spinBox(second)->value(), 42);
}

void tst_Aspects::integerAspectTakesEditsFromEverySpinBox()
{
    IntegerAspect aspect;
    aspect.setRange(0, 100);
    const auto first = createWidget(aspect);
    const auto second = createWidget(aspect);
    QVERIFY(spinBox(first));
    QVERIFY(spinBox(second));

    spinBox(first)->setValue(17);

    QCOMPARE(aspect.value(), 17);
    QCOMPARE(spinBox(second)->value(), 17);
}

void tst_Aspects::integerAspectKeepsUnscaledValue()
{
    IntegerAspect aspect;
    aspect.setRange(0, 10000);
    aspect.setDisplayScaleFactor(1000);
    const auto widget = createWidget(aspect);
    QVERIFY(spinBox(widget));

    aspect.setValue(1500);

    QCOMPARE(spinBox(widget)->value(), 1);
    QCOMPARE(aspect.value(), 1500);
}

void tst_Aspects::doubleAspectUpdatesEverySpinBox()
{
    DoubleAspect aspect;
    aspect.setRange(0, 100);
    const auto first = createWidget(aspect);
    const auto second = createWidget(aspect);
    QVERIFY(doubleSpinBox(first));
    QVERIFY(doubleSpinBox(second));

    aspect.setValue(4.5);

    QCOMPARE(doubleSpinBox(first)->value(), 4.5);
    QCOMPARE(doubleSpinBox(second)->value(), 4.5);
}

void tst_Aspects::doubleAspectTakesEditsFromEverySpinBox()
{
    DoubleAspect aspect;
    aspect.setRange(0, 100);
    const auto first = createWidget(aspect);
    const auto second = createWidget(aspect);
    QVERIFY(doubleSpinBox(first));
    QVERIFY(doubleSpinBox(second));

    doubleSpinBox(first)->setValue(1.5);

    QCOMPARE(aspect.value(), 1.5);
    QCOMPARE(doubleSpinBox(second)->value(), 1.5);
}

void tst_Aspects::doubleAspectKeepsUnroundedValue()
{
    DoubleAspect aspect;
    aspect.setRange(0, 100);
    const auto widget = createWidget(aspect);
    QVERIFY(doubleSpinBox(widget));
    QCOMPARE(doubleSpinBox(widget)->decimals(), 2);

    aspect.setValue(0.125);

    QCOMPARE(aspect.value(), 0.125);
}

void tst_Aspects::textDisplayGivesEveryWidgetALabel()
{
    TextDisplay display(nullptr, "Hello");
    const auto first = createWidget(display);
    const auto second = createWidget(display);

    QVERIFY(infoLabel(first));
    QVERIFY(infoLabel(second));
    QVERIFY(infoLabel(first) != infoLabel(second));
}

void tst_Aspects::textDisplayUpdatesEveryLabel()
{
    TextDisplay display(nullptr, "Hello");
    display.setIconType(InfoLabelType::None);
    const auto first = createWidget(display);
    const auto second = createWidget(display);
    QVERIFY(infoLabel(first));
    QVERIFY(infoLabel(second));

    display.setText("Changed");
    display.setIconType(InfoLabelType::Warning);

    QCOMPARE(infoLabel(first)->text(), "Changed");
    QCOMPARE(infoLabel(second)->text(), "Changed");
    QCOMPARE(infoLabel(first)->type(), InfoLabelType::Warning);
    QCOMPARE(infoLabel(second)->type(), InfoLabelType::Warning);
}

void tst_Aspects::multiSelectionAspectUpdatesEveryList()
{
    MultiSelectionAspect aspect;
    aspect.setAllValues({"a", "b", "c"});
    const auto first = createWidget(aspect);
    const auto second = createWidget(aspect);
    QVERIFY(listWidget(first));
    QVERIFY(listWidget(second));

    aspect.setValue({"a", "c"});

    QCOMPARE(checkedItems(listWidget(first)), QStringList({"a", "c"}));
    QCOMPARE(checkedItems(listWidget(second)), QStringList({"a", "c"}));
}

void tst_Aspects::multiSelectionAspectTakesEditsFromEveryList()
{
    MultiSelectionAspect aspect;
    aspect.setAllValues({"a", "b", "c"});
    const auto first = createWidget(aspect);
    const auto second = createWidget(aspect);
    QVERIFY(listWidget(first));
    QVERIFY(listWidget(second));

    listWidget(first)->item(1)->setCheckState(Qt::Checked);

    QCOMPARE(aspect.value(), QStringList({"b"}));
    QCOMPARE(checkedItems(listWidget(second)), QStringList({"b"}));
}

void tst_Aspects::multiSelectionAspectEditKeepsValueOrder()
{
    MultiSelectionAspect aspect;
    aspect.setAllValues({"a", "b", "c"});
    aspect.setValue({"c", "a", "x"});
    const auto widget = createWidget(aspect);
    QVERIFY(listWidget(widget));

    listWidget(widget)->item(1)->setCheckState(Qt::Checked);
    QCOMPARE(aspect.value(), QStringList({"c", "a", "x", "b"}));

    listWidget(widget)->item(0)->setCheckState(Qt::Unchecked);
    QCOMPARE(aspect.value(), QStringList({"c", "x", "b"}));
}

void tst_Aspects::multiSelectionAspectKeepsValueOrder()
{
    MultiSelectionAspect aspect;
    aspect.setAllValues({"a", "b", "c"});
    const auto widget = createWidget(aspect);
    QVERIFY(listWidget(widget));

    aspect.setValue({"c", "a"});

    QCOMPARE(aspect.value(), QStringList({"c", "a"}));
    QCOMPARE(checkedItems(listWidget(widget)), QStringList({"a", "c"}));
}

void tst_Aspects::filePathAspectDefaultValidation()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    FilePathAspect aspect;
    aspect.setExpectedKind(PathChooserKind::ExistingDirectory);
    const FancyLineEdit::AsyncValidationFunction validate = aspect.defaultValidationFunction();

    const FancyLineEdit::AsyncValidationResult existing = validate(dir.path()).result();
    if (!existing)
        QFAIL(qPrintable(existing.error()));

    const FancyLineEdit::AsyncValidationResult missing = validate(dir.filePath("missing")).result();
    QVERIFY(!missing);
    QVERIFY(missing.error().contains("does not exist"));

    QVERIFY(!validate({}).result());
}

void tst_Aspects::filePathAspectReportsValidity()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    FilePathAspect aspect;
    aspect.setExpectedKind(PathChooserKind::ExistingDirectory);
    const auto widget = createWidget(aspect);
    QVERIFY(pathChooser(widget));

    aspect.setValue(dir.path());
    QTRY_VERIFY(aspect.isValid());

    aspect.setValue(dir.filePath("missing"));
    QTRY_VERIFY(!aspect.isValid());
}

void tst_Aspects::filePathAspectAddsButtons()
{
    FilePathAspect aspect;
    QObject context;
    auto deadContext = std::make_unique<QObject>();
    int clicks = 0;
    aspect.addButton("Live", &context, [&clicks] { ++clicks; });
    aspect.addButton("Dead", deadContext.get(), [] {});
    deadContext.reset();

    const auto widget = createWidget(aspect);
    QVERIFY(pathChooser(widget));
    const QStringList texts = buttonTexts(pathChooser(widget));

    QVERIFY(texts.contains("Live"));
    QVERIFY(!texts.contains("Dead"));
    for (QAbstractButton *button : pathChooser(widget)->findChildren<QAbstractButton *>()) {
        if (button->text() == "Live")
            button->click();
    }
    QCOMPARE(clicks, 1);
}

void tst_Aspects::filePathAspectUpdatesShownPlaceholder()
{
    FilePathAspect aspect;
    const auto widget = createWidget(aspect);
    QVERIFY(pathChooser(widget));

    aspect.setPlaceHolderText("Hint");

    QCOMPARE(pathChooser(widget)->lineEdit()->placeholderText(), "Hint");

    FilePathAspect withDefault;
    withDefault.setDefaultValue("/default");
    const auto defaultWidget = createWidget(withDefault);
    QVERIFY(pathChooser(defaultWidget));

    withDefault.setPlaceHolderText("Hint");

    QCOMPARE(pathChooser(defaultWidget)->lineEdit()->placeholderText(),
             FilePath::fromUserInput("/default").toUserOutput());
}

void tst_Aspects::filePathAspectExpandsVolatilePath()
{
    FilePathAspect aspect;
    Environment env;
    env.set("QTC_TST_ASPECTS_DIR", "/qtc-tst-aspects");
    aspect.setEnvironment(env);
    aspect.setVolatileValue(HostOsInfo::isWindowsHost() ? "%QTC_TST_ASPECTS_DIR%/sub"
                                                        : "${QTC_TST_ASPECTS_DIR}/sub");

    QCOMPARE(aspect.expandedVolatilePath().path(), "/qtc-tst-aspects/sub");
}

QTEST_MAIN(tst_Aspects)

#include "tst_aspects.moc"
