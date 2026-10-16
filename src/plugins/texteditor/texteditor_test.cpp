// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifdef WITH_TESTS

#include "texteditor_test.h"

#include "storagesettings.h"
#include "displaysettings.h"
#include "tabsettings.h"
#include "textdocument.h"
#include "texteditor.h"
#include "marginsettings.h"
#include "textdocumentlayout.h"
#include "textmark.h"
#include "basefilefind.h"
#include "basehoverhandler.h"
#include "snippets/snippet.h"
#include "snippets/snippetparser.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/editormanager/ieditorfactory.h>
#include <coreplugin/externaltoolmanager.h>
#include <coreplugin/find/basetextfind.h>
#include <coreplugin/find/findplugin.h>
#include <coreplugin/find/ifindsupport.h>
#include <coreplugin/find/textfindconstants.h>
#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>

#include <utils/aggregate.h>
#include <utils/filepath.h>
#include <utils/multitextcursor.h>
#include <utils/plaintextedit/texteditorlayout.h>
#include <utils/temporarydirectory.h>
#include <utils/tooltip/tooltip.h>

#include <QAction>
#include <QApplication>
#include <QDropEvent>
#include <QHelpEvent>
#include <QImage>
#include <QLabel>
#include <QMainWindow>
#include <QMimeData>
#include <QPaintEvent>
#include <QScopeGuard>
#include <QScrollBar>
#include <QSignalSpy>
#include <QTest>
#include <QTextCursor>
#include <QTextDocument>

#include <memory>
#include <optional>

namespace TextEditor::Internal {

static QString tabPolicyToString(TabSettingsData::TabPolicy policy)
{
    switch (policy) {
    case TabSettingsData::SpacesOnlyTabPolicy:
        return QLatin1String("spacesOnlyPolicy");
    case TabSettingsData::TabsOnlyTabPolicy:
        return QLatin1String("tabsOnlyPolicy");
    }
    return QString();
}

static QString continuationAlignBehaviorToString(TabSettingsData::ContinuationAlignBehavior behavior)
{
    switch (behavior) {
    case TabSettingsData::NoContinuationAlign:
        return QLatin1String("noContinuation");
    case TabSettingsData::ContinuationAlignWithSpaces:
        return QLatin1String("spacesContinuation");
    case TabSettingsData::ContinuationAlignWithIndent:
        return QLatin1String("indentContinuation");
    }
    return QString();
}

struct TabSettingsFlags
{
    TabSettingsData::TabPolicy policy;
    TabSettingsData::ContinuationAlignBehavior behavior;
};

using IsClean = std::function<bool (TabSettingsFlags)>;

static void generateTestRows(const QLatin1String &name, const QString &text, IsClean isClean)
{
    const QList<TabSettingsData::TabPolicy> allPolicies = {
        TabSettingsData::SpacesOnlyTabPolicy,
        TabSettingsData::TabsOnlyTabPolicy,
    };
    const QList<TabSettingsData::ContinuationAlignBehavior> allbehaviors = {
        TabSettingsData::NoContinuationAlign,
        TabSettingsData::ContinuationAlignWithSpaces,
        TabSettingsData::ContinuationAlignWithIndent
    };

    const QLatin1Char splitter('_');
    const int indentSize = 3;

    for (auto policy : allPolicies) {
        for (auto behavior : allbehaviors) {
            const QString tag = tabPolicyToString(policy) + splitter
                    + continuationAlignBehaviorToString(behavior) + splitter
                    + name;
            QTest::newRow(tag.toLatin1().data())
                    << policy
                    << behavior
                    << text
                    << indentSize
                    << isClean({policy, behavior});
        }
    }
}

class TextEditorTest final : public QObject
{
    Q_OBJECT

    enum BlockSelectionMethod { Keyboard, MouseClick, MouseDrag };

private slots:
    void testIndentationClean_data();
    void testIndentationClean();
    void testIndentUnindent_data();
    void testIndentUnindent();
    void testMakefileForcesTabPolicy();
    void testTextDocumentChanged();
    void testIndentationGuides_data();
    void testIndentationGuides();
    void testTabStopOffset_data();
    void testTabStopOffset();
    void testBlockSelectionWithTabOffset_data();
    void testBlockSelectionWithTabOffset();
    void testTabStopArrayLimit();
    void testTabStopArrayBound_data();
    void testTabStopArrayBound();
    void testMoveLinesEndingInEmptyLine();
    void testMarkLeadingText();
};

void TextEditorTest::testIndentationClean_data()
{
    QTest::addColumn<TabSettingsData::TabPolicy>("policy");
    QTest::addColumn<TabSettingsData::ContinuationAlignBehavior>("behavior");
    QTest::addColumn<QString>("text");
    QTest::addColumn<int>("indentSize");
    QTest::addColumn<bool>("clean");

    generateTestRows(QLatin1String("emptyString"), QString::fromLatin1(""),
                     [](TabSettingsFlags) -> bool {
        return true;
    });

    generateTestRows(QLatin1String("spaceIndentation"), QString::fromLatin1("   f"),
                     [](TabSettingsFlags flags) -> bool {
        return flags.policy != TabSettingsData::TabsOnlyTabPolicy;
    });

    generateTestRows(QLatin1String("spaceIndentationGuessTabs"), QString::fromLatin1("   f\n\tf"),
                     [](TabSettingsFlags flags) -> bool {
        return flags.policy == TabSettingsData::SpacesOnlyTabPolicy;
    });

    generateTestRows(QLatin1String("tabIndentation"), QString::fromLatin1("\tf"),
                     [](TabSettingsFlags flags) -> bool {
        return flags.policy == TabSettingsData::TabsOnlyTabPolicy;
    });

    generateTestRows(QLatin1String("tabIndentationGuessTabs"), QString::fromLatin1("\tf\n\tf"),
                     [](TabSettingsFlags flags) -> bool {
        return flags.policy != TabSettingsData::SpacesOnlyTabPolicy;
    });

    generateTestRows(QLatin1String("doubleSpaceIndentation"), QString::fromLatin1("      f"),
                     [](TabSettingsFlags flags) -> bool {
        return flags.policy != TabSettingsData::TabsOnlyTabPolicy
                && flags.behavior != TabSettingsData::NoContinuationAlign;
    });

    generateTestRows(QLatin1String("doubleTabIndentation"), QString::fromLatin1("\t\tf"),
                     [](TabSettingsFlags flags) -> bool {
        return flags.policy == TabSettingsData::TabsOnlyTabPolicy
                && flags.behavior == TabSettingsData::ContinuationAlignWithIndent;
    });

    generateTestRows(QLatin1String("tabSpaceIndentation"), QString::fromLatin1("\t   f"),
                     [](TabSettingsFlags flags) -> bool {
        return flags.policy == TabSettingsData::TabsOnlyTabPolicy
                && flags.behavior == TabSettingsData::ContinuationAlignWithSpaces;
    });
}

void TextEditorTest::testIndentationClean()
{
    // fetch test data
    QFETCH(TabSettingsData::TabPolicy, policy);
    QFETCH(TabSettingsData::ContinuationAlignBehavior, behavior);
    QFETCH(QString, text);
    QFETCH(int, indentSize);
    QFETCH(bool, clean);

    const TabSettingsData settings(policy, indentSize, indentSize, behavior);
    const QTextDocument doc(text);
    const QTextBlock block = doc.firstBlock();

    QCOMPARE(settings.isIndentationClean(block, indentSize), clean);
}

void TextEditorTest::testIndentUnindent_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<int>("anchor");
    QTest::addColumn<int>("position");
    QTest::addColumn<TabSettingsData::TabPolicy>("policy");
    QTest::addColumn<int>("tabSize");
    QTest::addColumn<int>("indentSize");
    QTest::addColumn<bool>("doIndent");
    QTest::addColumn<QString>("expected");

    const auto Spaces = TabSettingsData::SpacesOnlyTabPolicy;
    const auto Tabs = TabSettingsData::TabsOnlyTabPolicy;

    // --- indent ---

    QTest::newRow("indent_emptyDocument")
        << QString() << 0 << 0 << Spaces << 8 << 4 << true << QString("    ");

    QTest::newRow("indent_singleLine_atStart_noSelection")
        << QString("foo") << 0 << 0 << Spaces << 8 << 4 << true << QString("    foo");

    QTest::newRow("indent_singleLine_fullSelection")
        << QString("foo") << 0 << 3 << Spaces << 8 << 4 << true << QString("    foo");

    QTest::newRow("indent_multiLine_selection")
        << QString("foo\nbar") << 0 << 7 << Spaces << 8 << 4 << true << QString("    foo\n    bar");

    QTest::newRow("indent_alreadyIndented_addsAnotherLevel")
        << QString("    foo") << 0 << 7 << Spaces << 8 << 4 << true << QString("        foo");

    QTest::newRow("indent_tabsOnlyPolicy")
        << QString("foo") << 0 << 3 << Tabs << 4 << 4 << true << QString("\tfoo");

    // --- unindent ---

    QTest::newRow("unindent_singleLine_atStart_noSelection")
        << QString("    foo") << 0 << 0 << Spaces << 8 << 4 << false << QString("foo");

    QTest::newRow("unindent_singleLine_fullSelection")
        << QString("    foo") << 0 << 7 << Spaces << 8 << 4 << false << QString("foo");

    QTest::newRow("unindent_multiLine_selection")
        << QString("    foo\n    bar") << 0 << 15 << Spaces << 8 << 4 << false
        << QString("foo\nbar");

    QTest::newRow("unindent_unindentedLine_isNoOp")
        << QString("foo") << 0 << 3 << Spaces << 8 << 4 << false << QString("foo");

    QTest::newRow("unindent_underAligned_stripsAllLeadingSpaces")
        << QString("  foo") << 0 << 5 << Spaces << 8 << 4 << false << QString("foo");

    QTest::newRow("unindent_overAligned_removesOneIndentStep")
        << QString("      foo") << 0 << 9 << Spaces << 8 << 4 << false << QString("  foo");

    QTest::newRow("unindent_tabIndented")
        << QString("\tfoo") << 0 << 4 << Tabs << 4 << 4 << false << QString("foo");

    // QTCREATORBUG-33260: unindent froze when m_indentSize > m_tabSize because
    // the old TabSettings::lineIndentPosition() helper could yield a negative
    // position, leaving the loop without forward progress.
    QTest::newRow("unindent_wideIndent_singleTab_QTCREATORBUG-33260")
        << QString("\trepeat") << 0 << 7 << Tabs << 8 << 16 << false << QString("repeat");

    // Three tabs with tabSize=8, indentSize=16 sit at column 24, which rounds down
    // to indent column 16; unindenting steps down to column 0 by removing two tabs.
    QTest::newRow("unindent_wideIndent_threeTabs_QTCREATORBUG-33260")
        << QString("\t\t\trepeat") << 0 << 9 << Tabs << 8 << 16 << false << QString("\trepeat");
}

void TextEditorTest::testIndentUnindent()
{
    QFETCH(QString, input);
    QFETCH(int, anchor);
    QFETCH(int, position);
    QFETCH(TabSettingsData::TabPolicy, policy);
    QFETCH(int, tabSize);
    QFETCH(int, indentSize);
    QFETCH(bool, doIndent);
    QFETCH(QString, expected);

    TextDocument doc;
    doc.setPlainText(input);

    TabSettingsData settings(policy, tabSize, indentSize, TabSettingsData::ContinuationAlignWithSpaces);
    settings.m_autoDetect = false;
    doc.setTabSettings(settings);

    QTextCursor cursor(doc.document());
    cursor.setPosition(anchor);
    cursor.setPosition(position, QTextCursor::KeepAnchor);
    Utils::MultiTextCursor multi({cursor});

    if (doIndent)
        doc.indent(multi);
    else
        doc.unindent(multi);

    QCOMPARE(doc.plainText(), expected);
}

void TextEditorTest::testMakefileForcesTabPolicy()
{
    // Makefiles are tab-sensitive, so the tab policy is forced to tabs
    // regardless of the configured code style - including for files that only
    // inherit text/x-makefile, such as debian/rules (QTCREATORBUG-3408).
    TabSettingsData spaces(TabSettingsData::SpacesOnlyTabPolicy, 8, 8,
                           TabSettingsData::ContinuationAlignWithSpaces);
    spaces.m_autoDetect = false;

    TextDocument makefile;
    makefile.setMimeType("text/x-makefile");
    makefile.setTabSettings(spaces);
    QCOMPARE(makefile.tabSettings().m_tabPolicy, TabSettingsData::TabsOnlyTabPolicy);

    // A non-makefile keeps the configured policy.
    TextDocument plain;
    plain.setMimeType("text/plain");
    plain.setTabSettings(spaces);
    QCOMPARE(plain.tabSettings().m_tabPolicy, TabSettingsData::SpacesOnlyTabPolicy);
}

void TextEditorTest::testTextDocumentChanged()
{
    TextEditorWidget widget;
    QSignalSpy signalSpy(&widget, &TextEditorWidget::textDocumentChanged);
    const TextDocumentPtr document(new TextDocument);

    widget.setTextDocument(document);

    QCOMPARE(signalSpy.count(), 1);
    QCOMPARE(widget.textDocument(), document.data());
}

void TextEditorTest::testIndentationGuides_data()
{
    QTest::addColumn<QString>("text");
    QTest::addColumn<QString>("prefix");
    QTest::addColumn<int>("cursorWidth");

    const QList<QString> texts = {"", "  ", "\t", "\t\tvalue", " \t \tvalue"};
    for (int i = 0; i < texts.size(); ++i) {
        for (const QString &prefix : {QString(), QString("+")}) {
            if (!prefix.isEmpty() && texts.at(i).contains('\t'))
                continue;
            for (int cursorWidth : {1, 2, 4}) {
                const QString name = QString("text%1-prefix%2-cursor%3")
                                         .arg(i).arg(prefix.size()).arg(cursorWidth);
                QTest::newRow(qPrintable(name)) << texts.at(i) << prefix << cursorWidth;
            }
        }
    }
}

void TextEditorTest::testIndentationGuides()
{
    QFETCH(QString, text);
    QFETCH(QString, prefix);
    QFETCH(int, cursorWidth);

    class GuideEditorWidget : public TextEditorWidget
    {
    public:
        using TextEditorWidget::setVisualIndentOffset;
        using TextEditorWidget::triggerPendingUpdates;
    } widget;
    widget.setTextDocument(TextDocumentPtr(new TextDocument));
    widget.triggerPendingUpdates();
    widget.setVisualIndentOffset(prefix.size());
    widget.setCursorWidth(cursorWidth);
    widget.resize(600, 200);

    TabSettingsData settings = widget.textDocument()->tabSettings();
    settings.m_autoDetect = false;
    settings.m_tabSize = 8;
    settings.m_indentSize = 4;
    widget.textDocument()->setTabSettings(settings);
    const QString neighbour = prefix + QString(16, ' ') + "value";
    widget.setPlainText(neighbour + '\n' + prefix + text + '\n' + neighbour);

    DisplaySettingsData display = widget.displaySettings();
    display.m_visualizeIndent = false;
    display.m_visualizeWhitespace = false;
    display.m_textWrapping = false;
    display.m_displayMinimap = false;
    widget.setDisplaySettings(display);
    widget.show();
    const QImage withoutGuides = widget.viewport()->grab().toImage();
    display.m_visualizeIndent = true;
    widget.setDisplaySettings(display);
    const QImage withGuides = widget.viewport()->grab().toImage();

    QTextCursor reference(widget.document()->firstBlock());
    const QTextBlock target = reference.block().next();
    QTextCursor targetCursor(target);
    const QRect targetRect = widget.cursorRect(targetCursor);
    const qreal scale = withGuides.devicePixelRatio();
    const int tolerance = qMax(1, qRound(scale));
    const int depth = text.simplified().isEmpty() ? 16 : settings.indentationColumn(text);
    for (int column = 0; column < depth; column += settings.m_indentSize) {
        reference.setPosition(prefix.size() + column);
        const int x = qRound(widget.cursorRect(reference).right() * scale);
        bool foundGuide = false;
        for (int dx = -tolerance; dx <= tolerance; ++dx) {
            int changedPixels = 0;
            for (int y = qRound((targetRect.top() + 2) * scale);
                 y < qRound((targetRect.bottom() - 2) * scale); ++y) {
                if (withGuides.pixel(x + dx, y) != withoutGuides.pixel(x + dx, y))
                    ++changedPixels;
            }
            foundGuide |= changedPixels >= targetRect.height() * scale / 2;
        }
        QVERIFY2(foundGuide, qPrintable(QString("Missing guide at column %1").arg(column)));
    }
}

void TextEditorTest::testTabStopOffset_data()
{
    QTest::addColumn<QString>("prefix");
    QTest::addColumn<QString>("text");
    QTest::addColumn<int>("tabSize");
    QTest::addColumn<bool>("replaceDocument");

    for (const QString &prefix : {QString(), QString("+"), QString("-"), QString(" ")}) {
        const QList<QString> texts = {"\ttext", " \t \ttext", "    \ttext",
                                      QString(32, '\t') + " \ttext"};
        for (const QString &text : texts) {
            for (int tabSize : {1, 4, 8}) {
                for (bool replaceDocument : {false, true}) {
                    const QString name = QString("%1-%2-%3-%4")
                                             .arg(prefix, text).arg(tabSize).arg(replaceDocument);
                    QTest::newRow(qPrintable(name))
                        << prefix << text << tabSize << replaceDocument;
                }
            }
        }
    }
}

void TextEditorTest::testTabStopOffset()
{
    QFETCH(QString, prefix);
    QFETCH(QString, text);
    QFETCH(int, tabSize);
    QFETCH(bool, replaceDocument);

    class OffsetEditorWidget : public TextEditorWidget
    {
    public:
        using TextEditorWidget::setVisualIndentOffset;
        using TextEditorWidget::triggerPendingUpdates;
    } widget;
    widget.setTextDocument(TextDocumentPtr(new TextDocument));
    if (replaceDocument) {
        widget.setVisualIndentOffset(prefix.size());
        widget.setTextDocument(TextDocumentPtr(new TextDocument));
    }
    widget.triggerPendingUpdates();
    widget.setLineWrapMode(Utils::PlainTextEdit::NoWrap);
    TabSettingsData settings = widget.textDocument()->tabSettings();
    settings.m_autoDetect = false;
    settings.m_tabSize = tabSize;
    widget.textDocument()->setTabSettings(settings);

    const int columns = settings.columnAt(text, text.size());
    widget.setPlainText(prefix + text + '\n' + prefix + QString(columns, ' '));
    if (!replaceDocument)
        widget.setVisualIndentOffset(prefix.size());

    const QTextBlock actualBlock = widget.document()->firstBlock();
    const QTextBlock expectedBlock = actualBlock.next();
    widget.editorLayout()->ensureBlockLayout(actualBlock);
    widget.editorLayout()->ensureBlockLayout(expectedBlock);
    const QTextLine actual = widget.editorLayout()->blockLayout(actualBlock)->lineAt(0);
    const QTextLine expected = widget.editorLayout()->blockLayout(expectedBlock)->lineAt(0);
    for (int position = 0; position <= text.size(); ++position) {
        const int column = text.count('\t') > 20
            ? settings.columnAt(prefix + text, prefix.size() + position) - prefix.size()
            : settings.columnAt(text, position);
        const qreal actualX = actual.cursorToX(prefix.size() + position);
        const qreal expectedX = expected.cursorToX(prefix.size() + column);
        QVERIFY2(qAbs(actualX - expectedX) < 0.1,
                 qPrintable(QString("Position %1: %2 != %3")
                                .arg(position).arg(actualX).arg(expectedX)));
    }
}

void TextEditorTest::testBlockSelectionWithTabOffset_data()
{
    QTest::addColumn<QString>("prefix");
    QTest::addColumn<int>("tabSize");
    QTest::addColumn<int>("tabCount");
    QTest::addColumn<int>("method");
    QTest::addColumn<bool>("reverse");
    for (const QString &prefix : QStringList{"", "+", "-", " ", "++"}) {
        for (int tabSize : {4, 8}) {
            for (int tabCount : {1, 20, 21}) {
                for (int method : {Keyboard, MouseClick, MouseDrag}) {
                    for (bool reverse : {false, true}) {
                        QTest::addRow("%s-%d-%d-%d-%d", qPrintable(prefix), tabSize,
                                      tabCount, method, reverse)
                            << prefix << tabSize << tabCount << method << reverse;
                    }
                }
            }
        }
    }
}

void TextEditorTest::testBlockSelectionWithTabOffset()
{
    QFETCH(QString, prefix);
    QFETCH(int, tabSize);
    QFETCH(int, tabCount);
    QFETCH(int, method);
    QFETCH(bool, reverse);

    class OffsetEditorWidget : public TextEditorWidget
    {
    public:
        using TextEditorWidget::setVisualIndentOffset;
        using TextEditorWidget::triggerPendingUpdates;
    } widget;
    widget.setTextDocument(TextDocumentPtr(new TextDocument));
    widget.triggerPendingUpdates();
    widget.setLineWrapMode(Utils::PlainTextEdit::NoWrap);
    TabSettingsData settings = widget.textDocument()->tabSettings();
    settings.m_autoDetect = false;
    settings.m_tabSize = tabSize;
    widget.textDocument()->setTabSettings(settings);
    const QString tabs(tabCount, '\t');
    const int spaces = tabCount <= 20 ? tabCount * tabSize
        : settings.columnAt(prefix + tabs, prefix.size() + tabCount) - prefix.size();
    widget.setPlainText(prefix + tabs + "x\n" + prefix + QString(spaces, ' ') + 'x');
    widget.setVisualIndentOffset(prefix.size());
    widget.setReadOnly(true);
    widget.resize(640, 240);
    widget.show();

    const QTextBlock first = widget.document()->firstBlock();
    const QTextBlock second = first.next();
    const int firstX = first.position() + prefix.size() + tabCount;
    const int secondX = second.position() + prefix.size() + spaces;
    const int anchor = reverse ? secondX : firstX;
    const int target = reverse ? firstX : secondX;
    QTextCursor cursor(widget.document());
    cursor.setPosition(anchor);
    if (method == Keyboard)
        cursor.setPosition(anchor + 1, QTextCursor::KeepAnchor);
    widget.setTextCursor(cursor);

    if (method == Keyboard) {
        QTest::keyClick(&widget, reverse ? Qt::Key_Up : Qt::Key_Down,
                        Qt::AltModifier | Qt::ShiftModifier);
    } else {
        cursor.setPosition(target + 1);
        const QPoint point = widget.cursorRect(cursor).center();
        if (method == MouseClick) {
            QTest::mouseClick(widget.viewport(), Qt::LeftButton,
                              Qt::AltModifier | Qt::ShiftModifier, point);
        } else {
            QMouseEvent event(QEvent::MouseMove, point, widget.viewport()->mapToGlobal(point),
                              Qt::NoButton, Qt::LeftButton, Qt::AltModifier);
            QCoreApplication::sendEvent(widget.viewport(), &event);
            QMouseEvent endDrag(QEvent::MouseMove, point, widget.viewport()->mapToGlobal(point),
                                Qt::NoButton, Qt::NoButton, Qt::NoModifier);
            QCoreApplication::sendEvent(widget.viewport(), &endDrag);
        }
    }
    const QList<QTextCursor> cursors = widget.multiTextCursor().cursors();
    QCOMPARE(cursors.size(), 2);
    for (const QTextCursor &selection : cursors)
        QCOMPARE(selection.selectedText(), QString("x"));
}

void TextEditorTest::testTabStopArrayLimit()
{
    QTextDocument document;
    auto layout = new Utils::PlainTextDocumentLayout(&document);
    document.setDocumentLayout(layout);
    layout->setTabStopOffset(1);
    document.setPlainText('+' + QString(21, '\t'));
    layout->ensureBlockLayout(document.firstBlock());
    QVERIFY(layout->blockLayout(document.firstBlock())->textOption().tabArray().isEmpty());
}

void TextEditorTest::testTabStopArrayBound_data()
{
    QTest::addColumn<QString>("beforeTail");
    QTest::addColumn<int>("tabSize");
    QTest::addColumn<bool>("shifted");
    const QList<QString> texts = {"\t", QString(1000, ' ') + '\t', QString(20, '\t'),
                                  QString(21, '\t'), QString(128, '\t'), QString(100000, '\t')};
    for (int i = 0; i < texts.size(); ++i) {
        for (int tabSize : {1, 4, 8}) {
            const QString name = QString("text%1-tab%2").arg(i).arg(tabSize);
            QTest::newRow(qPrintable(name)) << texts.at(i) << tabSize << (i == 0 || i == 2);
        }
    }
}

void TextEditorTest::testTabStopArrayBound()
{
    QFETCH(QString, beforeTail);
    QFETCH(int, tabSize);
    QFETCH(bool, shifted);

    class OffsetEditorWidget : public TextEditorWidget
    {
    public:
        using TextEditorWidget::setVisualIndentOffset;
        using TextEditorWidget::triggerPendingUpdates;
    } widget;
    widget.setTextDocument(TextDocumentPtr(new TextDocument));
    widget.triggerPendingUpdates();
    widget.setVisualIndentOffset(1);
    widget.setLineWrapMode(Utils::PlainTextEdit::NoWrap);
    TabSettingsData settings = widget.textDocument()->tabSettings();
    settings.m_autoDetect = false;
    settings.m_tabSize = tabSize;
    widget.textDocument()->setTabSettings(settings);

    const int columns = settings.columnAt(beforeTail, beforeTail.size());
    widget.setPlainText('+' + beforeTail + QString(100000, 'x') + '\n'
                        + '+' + QString(columns, ' '));
    const QTextBlock actualBlock = widget.document()->firstBlock();
    const QTextBlock expectedBlock = actualBlock.next();
    widget.editorLayout()->ensureBlockLayout(actualBlock);
    widget.editorLayout()->ensureBlockLayout(expectedBlock);
    const QTextLayout *actualLayout = widget.editorLayout()->blockLayout(actualBlock);
    const QList<qreal> stops = actualLayout->textOption().tabArray();
    QVERIFY(stops.size() <= 20);
    QCOMPARE(stops.isEmpty(), !shifted);

    const QTextLine actual = actualLayout->lineAt(0);
    const QTextLine expected = widget.editorLayout()->blockLayout(expectedBlock)->lineAt(0);
    for (int position : {beforeTail.indexOf('\t'), beforeTail.lastIndexOf('\t')}) {
        const int column = shifted ? settings.columnAt(beforeTail, position + 1)
            : settings.columnAt('+' + beforeTail, position + 2) - 1;
        QVERIFY(qAbs(actual.cursorToX(position + 2) - expected.cursorToX(column + 1)) < 0.1);
    }
}

// Regression test for QTCREATORBUG-34933.
void TextEditorTest::testMoveLinesEndingInEmptyLine()
{
    const QString input = "a\nb\n\nc\nd\ne";
    QString title = "move_lines.txt";
    Core::IEditor *editor = Core::EditorManager::openEditorWithContents(
        Core::Constants::K_DEFAULT_TEXT_EDITOR_ID, &title, input.toUtf8());
    QVERIFY(editor);
    const QScopeGuard cleanup([&] { Core::EditorManager::closeEditors({editor}, false); });
    auto baseEditor = qobject_cast<BaseTextEditor *>(editor);
    QVERIFY(baseEditor);
    TextEditorWidget *editorWidget = baseEditor->editorWidget();
    QVERIFY(editorWidget);

    // The lines "b" and "", selected from the start of "b" to the start of "c".
    QTextCursor cursor = editorWidget->textCursor();
    cursor.setPosition(2);
    cursor.setPosition(5, QTextCursor::KeepAnchor);
    editorWidget->setTextCursor(cursor);

    editorWidget->moveLineDown();
    QCOMPARE(editorWidget->textDocument()->plainText(), QString("a\nc\nb\n\nd\ne"));
    editorWidget->moveLineDown();
    QCOMPARE(editorWidget->textDocument()->plainText(), QString("a\nc\nd\nb\n\ne"));
    editorWidget->moveLineUp();
    editorWidget->moveLineUp();
    QCOMPARE(editorWidget->textDocument()->plainText(), input);
}

void TextEditorTest::testMarkLeadingText()
{
    TextEditorWidget widget;
    const TextDocumentPtr document(new TextDocument);
    document->setPlainText("one\ntwo\nthree\n");
    widget.setTextDocument(document);
    widget.resize(1200, 300);
    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));

    TextMark mark(document.data(), 2, {"Test", "Test.LeadingText"});
    const int before = widget.extraAreaWidth();
    const QString text = "Someone, 2 days ago";
    const int textWidth = widget.extraArea()->fontMetrics().horizontalAdvance(text);
    mark.setLeadingText(text);
    QTRY_VERIFY(widget.extraArea()->width() >= before + textWidth);
    int markWidth = 0;
    widget.extraAreaWidth(&markWidth);

    // In the row of the line that has it, and not in the one above.
    const QImage area = widget.extraArea()->grab().toImage();
    const QRgb background = area.pixel(area.width() / 2, area.height() - 1);
    const auto inks = [&](int line) {
        QTextCursor cursor(document->document()->findBlockByNumber(line - 1));
        const QRect row = widget.cursorRect(cursor);
        const qreal ratio = area.devicePixelRatio();
        int count = 0;
        for (int y = qRound(row.top() * ratio); y <= qRound(row.bottom() * ratio); ++y) {
            for (int x = qRound(markWidth * ratio); x < qRound((markWidth + textWidth) * ratio);
                 ++x) {
                if (area.pixel(x, y) != background)
                    ++count;
            }
        }
        return count;
    };
    QVERIFY(inks(2) > 0);
    QCOMPARE(inks(1), 0);

    // A long one leaves room for the code.
    const int shortWidth = widget.extraArea()->width();
    mark.setLeadingText(text.repeated(20));
    QTRY_VERIFY(widget.extraArea()->width() > shortWidth);
    QVERIFY(widget.extraArea()->width() < before + widget.width() / 2);

    // A hidden mark draws nothing, so it takes no room.
    mark.setVisible(false);
    QTRY_COMPARE(widget.extraArea()->width(), before);
    mark.setVisible(true);
    QTRY_VERIFY(widget.extraArea()->width() > before);

    // The marks of one line share its text.
    const QString second = "Other, 3 days ago";
    TextMark other(document.data(), 2, {"Test", "Test.LeadingText"});
    other.setLeadingText(second);
    const int joinedWidth = widget.extraArea()->fontMetrics().horizontalAdvance(
        text + ' ' + second);
    QTRY_VERIFY(widget.extraArea()->width() >= before + joinedWidth);
    other.setLeadingText({});

    mark.setLeadingText({});
    QTRY_COMPARE(widget.extraArea()->width(), before);
}

QObject *createTextEditorTest()
{
    return new TextEditorTest;
}

// Regression test for QTCREATORBUG-24565.
class CleanWhitespaceTest final : public QObject
{
    Q_OBJECT

private slots:
    void testCleanWhitespace_data();
    void testCleanWhitespace();
};

void CleanWhitespaceTest::testCleanWhitespace_data()
{
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<QString>("ignoreFileTypes");
    QTest::addColumn<bool>("skipTrailingWhitespace");
    QTest::addColumn<int>("tabPolicy");
    QTest::addColumn<QString>("input");
    QTest::addColumn<QString>("expected");
    QTest::addColumn<bool>("modified"); // whether cleaning is expected to change the document

    const QString tabBlankTab = "\t \t";  // trailing whitespace, as in the Squish test
    const QString tripleTab = "\t\t\t";   // whitespace-only line, as in the Squish test
    const QString defaultIgnore = "*.md, *.MD, Makefile";
    const int tabsOnly = TabSettingsData::TabsOnlyTabPolicy;
    const int spacesOnly = TabSettingsData::SpacesOnlyTabPolicy;

    // A regular source file: trailing whitespace is stripped and the
    // whitespace-only line is emptied (the QTCREATORBUG-24565 scenario).
    QTest::newRow("regularFile")
        << "main.cpp" << defaultIgnore << true << tabsOnly
        << ("foo" + tabBlankTab + "\n" + tripleTab + "\nbar\n")
        << QString("foo\n\nbar\n") << true;

    // A file matching the ignore list is left completely untouched: neither
    // trailing whitespace nor indentation (here the whitespace-only line) is
    // cleaned. "Skip clean whitespace for file types" skips all whitespace
    // cleanup, not just trailing (QTCREATORBUG-32894).
    QTest::newRow("ignoredExtension")
        << "notes.md" << defaultIgnore << true << tabsOnly
        << ("foo" + tabBlankTab + "\n" + tripleTab + "\nbar\n")
        << ("foo" + tabBlankTab + "\n" + tripleTab + "\nbar\n") << false;

    // The ignore list also matches plain file names without an extension. With
    // nothing left to clean, the document stays unmodified (Squish checked this
    // via the editor's "*" modification marker).
    QTest::newRow("ignoredName")
        << "Makefile" << defaultIgnore << true << tabsOnly
        << ("foo" + tabBlankTab + "\n")
        << ("foo" + tabBlankTab + "\n") << false;

    // A Makefile (on the ignore list) keeps its mandatory tab indentation even
    // under a Spaces-Only policy that would otherwise rewrite it to spaces
    // (QTCREATORBUG-32894).
    QTest::newRow("makefileTabIndentIsKept")
        << "Makefile" << defaultIgnore << true << spacesOnly
        << QString("all:\n\tgcc foo.c\n")
        << QString("all:\n\tgcc foo.c\n") << false;

    // A regular file under a Spaces-Only policy: tab indentation IS rewritten to
    // spaces (the counterpart to the Makefile case above).
    QTest::newRow("regularFileTabIndentConverted")
        << "main.cpp" << defaultIgnore << true << spacesOnly
        << QString("void f()\n{\n\tint x;\n}\n")
        << QString("void f()\n{\n    int x;\n}\n") << true;

    // With "skip trailing whitespace" disabled, trailing whitespace is trimmed
    // even for files on the ignore list (the counter-intuitive sense of the flag).
    QTest::newRow("alwaysTrimOverridesIgnoreList")
        << "notes.md" << defaultIgnore << false << tabsOnly
        << ("foo" + tabBlankTab + "\n")
        << QString("foo\n") << true;

    // With a Tabs-Only policy, a tab-indented code line must keep its tab and
    // must not be rewritten to spaces (QTCREATORBUG-33670).
    QTest::newRow("tabIndentIsKept")
        << "main.cpp" << defaultIgnore << true << tabsOnly
        << QString("if (x)\n\tfoo();\n")
        << QString("if (x)\n\tfoo();\n") << false;

    // Nested tab indentation stays as tabs, not tab-plus-spaces
    // (QTCREATORBUG-33670).
    QTest::newRow("nestedTabIndentIsKept")
        << "main.cpp" << defaultIgnore << true << tabsOnly
        << QString("if (x)\n\tif (y)\n\t\tfoo();\n")
        << QString("if (x)\n\tif (y)\n\t\tfoo();\n") << false;
}

void CleanWhitespaceTest::testCleanWhitespace()
{
    QFETCH(QString, fileName);
    QFETCH(QString, ignoreFileTypes);
    QFETCH(bool, skipTrailingWhitespace);
    QFETCH(int, tabPolicy);
    QFETCH(QString, input);
    QFETCH(QString, expected);
    QFETCH(bool, modified);

    TextDocument doc;
    doc.setFilePath(Utils::FilePath::fromString(fileName));

    StorageSettingsData settings;
    settings.m_ignoreFileTypes = ignoreFileTypes;
    settings.m_skipTrailingWhitespace = skipTrailingWhitespace;
    settings.m_cleanWhitespace = true;
    settings.m_inEntireDocument = true;
    settings.m_cleanIndentation = true;
    settings.m_addFinalNewLine = false;
    doc.setStorageSettings(settings);

    // Use a deterministic tab configuration instead of autodetection.
    TabSettingsData tabSettings(TabSettingsData::TabPolicy(tabPolicy), 4, 4,
                                TabSettingsData::ContinuationAlignWithSpaces);
    tabSettings.m_autoDetect = false;
    doc.setTabSettings(tabSettings);

    doc.setPlainText(input);
    doc.document()->setModified(false);

    QTextCursor cursor(doc.document());
    doc.cleanWhitespace(cursor);

    QCOMPARE(doc.plainText(), expected);
    QCOMPARE(doc.document()->isModified(), modified);
}

QObject *createCleanWhitespaceTest()
{
    return new CleanWhitespaceTest;
}

class SortLinesTest final : public QObject
{
    Q_OBJECT

private slots:
    void testSortLines();
};

void SortLinesTest::testSortLines()
{
    const QString input = "bbb\n\n aa\nAAA\naaa\n1A\n1a\naa\n!\n_\na";
    const QString expected = "\n aa\n!\n1A\n1a\nAAA\n_\na\naa\naaa\nbbb";

    QString title = "unsorted.txt";
    Core::IEditor *editor = Core::EditorManager::openEditorWithContents(
        Core::Constants::K_DEFAULT_TEXT_EDITOR_ID, &title, input.toUtf8());
    QVERIFY(editor);
    auto baseEditor = qobject_cast<BaseTextEditor *>(editor);
    QVERIFY(baseEditor);
    TextEditorWidget *editorWidget = baseEditor->editorWidget();
    QVERIFY(editorWidget);

    editorWidget->selectAll();
    editorWidget->sortLines();

    QCOMPARE(editorWidget->textDocument()->plainText(), expected);

    Core::EditorManager::closeEditors({editor}, false);
}

QObject *createSortLinesTest()
{
    return new SortLinesTest;
}

// Regression test for QTCREATORBUG-5757.
class SelectAllTest final : public QObject
{
    Q_OBJECT

private slots:
    void testCursorAfterSelectAll_data();
    void testCursorAfterSelectAll();
};

void SelectAllTest::testCursorAfterSelectAll_data()
{
    QTest::addColumn<int>("key");
    QTest::addColumn<int>("expected"); // 0 = at start, 1 = at end, 2 = only collapsed

    QTest::newRow("Left collapses to start") << int(Qt::Key_Left) << 0;
    QTest::newRow("Right collapses to end") << int(Qt::Key_Right) << 1;
    QTest::newRow("Down stays at end") << int(Qt::Key_Down) << 1;
    QTest::newRow("Up collapses") << int(Qt::Key_Up) << 2;
}

void SelectAllTest::testCursorAfterSelectAll()
{
    QFETCH(int, key);
    QFETCH(int, expected);

    const QString content = "first line\nsecond line\nthird line";
    QString title = "select_all.txt";
    Core::IEditor *editor = Core::EditorManager::openEditorWithContents(
        Core::Constants::K_DEFAULT_TEXT_EDITOR_ID, &title, content.toUtf8());
    QVERIFY(editor);
    auto baseEditor = qobject_cast<BaseTextEditor *>(editor);
    QVERIFY(baseEditor);
    TextEditorWidget *editorWidget = baseEditor->editorWidget();
    QVERIFY(editorWidget);

    const int size = editorWidget->textDocument()->plainText().length();

    editorWidget->selectAll();
    QTextCursor cursor = editorWidget->textCursor();
    QVERIFY(cursor.hasSelection());
    QCOMPARE(cursor.selectionStart(), 0);
    QCOMPARE(cursor.selectionEnd(), size);
    QCOMPARE(cursor.position(), size);

    QTest::keyClick(editorWidget, Qt::Key(key));

    cursor = editorWidget->textCursor();
    QVERIFY(!cursor.hasSelection());
    switch (expected) {
    case 0:
        QCOMPARE(cursor.position(), 0);
        break;
    case 1:
        QCOMPARE(cursor.position(), size);
        break;
    case 2:
        QCOMPARE(cursor.selectionStart(), cursor.selectionEnd());
        break;
    }

    Core::EditorManager::closeEditors({editor}, false);
}

QObject *createSelectAllTest()
{
    return new SelectAllTest;
}

class UndoCursorTest final : public QObject
{
    Q_OBJECT

private slots:
    void testSelectionRestoredOnUndo();
    void testMultiCursorRestoredOnUndo();
    void testMultiCursorTypingUndo();
    void testSelectionRestoredOnRedo();
    void testSelectAllRestoredOnUndo();
    void testEditAfterUndo();
    void testDropUndo();
    void testJoinedMoveLineUndo();
    void testJoinedIndentUndo();
    void testUndoRedoInDuplicate();
    void testUndoInOtherView();
    void testUndoOfEditInOtherEditor();
    void testUndoOfEditWhileNonTextEditorIsCurrent();
    void testUndoInEmbeddedWidget();
    void testUndoInFocusedWidget();

private:
    TextEditorWidget *openEditor(const QByteArray &contents);

    Core::IEditor *m_editor = nullptr;
};

TextEditorWidget *UndoCursorTest::openEditor(const QByteArray &contents)
{
    QString title = "undo_cursor.txt";
    m_editor = Core::EditorManager::openEditorWithContents(
        Core::Constants::K_DEFAULT_TEXT_EDITOR_ID, &title, contents);
    if (!m_editor)
        return nullptr;
    auto baseEditor = qobject_cast<BaseTextEditor *>(m_editor);
    return baseEditor ? baseEditor->editorWidget() : nullptr;
}

void UndoCursorTest::testSelectionRestoredOnUndo()
{
    const QString contents = "first line\nsecond line";
    TextEditorWidget *editorWidget = openEditor(contents.toUtf8());
    QVERIFY(editorWidget);
    const QScopeGuard cleanup([this] { Core::EditorManager::closeEditors({m_editor}, false); });

    QTextCursor cursor = editorWidget->textCursor();
    cursor.setPosition(0);
    cursor.setPosition(5, QTextCursor::KeepAnchor);
    editorWidget->setTextCursor(cursor);

    QTest::keyClick(editorWidget, Qt::Key_X);
    QCOMPARE(editorWidget->textDocument()->plainText(), QString("x line\nsecond line"));

    editorWidget->undo();

    QCOMPARE(editorWidget->textDocument()->plainText(), contents);
    cursor = editorWidget->textCursor();
    QVERIFY(cursor.hasSelection());
    QCOMPARE(cursor.anchor(), 0);
    QCOMPARE(cursor.position(), 5);
}

void UndoCursorTest::testMultiCursorRestoredOnUndo()
{
    const QString contents = "first line\nsecond line";
    TextEditorWidget *editorWidget = openEditor(contents.toUtf8());
    QVERIFY(editorWidget);
    const QScopeGuard cleanup([this] { Core::EditorManager::closeEditors({m_editor}, false); });

    QTextDocument *document = editorWidget->document();
    QTextCursor first(document);
    first.setPosition(0);
    first.setPosition(5, QTextCursor::KeepAnchor);
    QTextCursor second(document);
    second.setPosition(11);
    second.setPosition(17, QTextCursor::KeepAnchor);
    editorWidget->setMultiTextCursor(Utils::MultiTextCursor({first, second}));

    QTest::keyClick(editorWidget, Qt::Key_X);
    QCOMPARE(editorWidget->textDocument()->plainText(), QString("x line\nx line"));

    editorWidget->undo();

    QCOMPARE(editorWidget->textDocument()->plainText(), contents);
    const QList<QTextCursor> cursors = editorWidget->multiTextCursor().cursors();
    QCOMPARE(cursors.size(), 2);
    QCOMPARE(cursors.at(0).anchor(), 0);
    QCOMPARE(cursors.at(0).position(), 5);
    QCOMPARE(cursors.at(1).anchor(), 11);
    QCOMPARE(cursors.at(1).position(), 17);
}

void UndoCursorTest::testMultiCursorTypingUndo()
{
    const QString contents = "a\nb";
    TextEditorWidget *editorWidget = openEditor(contents.toUtf8());
    QVERIFY(editorWidget);
    const QScopeGuard cleanup([this] { Core::EditorManager::closeEditors({m_editor}, false); });

    // A cursor at the end of each line.
    QTextDocument *document = editorWidget->document();
    QTextCursor first(document);
    first.setPosition(1);
    QTextCursor second(document);
    second.setPosition(3);
    editorWidget->setMultiTextCursor(Utils::MultiTextCursor({first, second}));

    // Typing in a row is undone in one step, as with a single cursor.
    QTest::keyClicks(editorWidget, "xyz");
    QCOMPARE(editorWidget->textDocument()->plainText(), QString("axyz\nbxyz"));
    editorWidget->undo();
    QCOMPARE(editorWidget->textDocument()->plainText(), contents);
    QCOMPARE(editorWidget->multiTextCursor().cursorCount(), 2);

    // Moving the cursors ends the step.
    QTest::keyClicks(editorWidget, "xy");
    QTest::keyClick(editorWidget, Qt::Key_Left);
    QTest::keyClicks(editorWidget, "z");
    QCOMPARE(editorWidget->textDocument()->plainText(), QString("axzy\nbxzy"));
    editorWidget->undo();
    QCOMPARE(editorWidget->textDocument()->plainText(), QString("axy\nbxy"));
    editorWidget->undo();
    QCOMPARE(editorWidget->textDocument()->plainText(), contents);
}

void UndoCursorTest::testSelectionRestoredOnRedo()
{
    const QString contents = "first line\nsecond line";
    TextEditorWidget *editorWidget = openEditor(contents.toUtf8());
    QVERIFY(editorWidget);
    const QScopeGuard cleanup([this] { Core::EditorManager::closeEditors({m_editor}, false); });

    QTextCursor cursor = editorWidget->textCursor();
    cursor.setPosition(0);
    cursor.setPosition(5, QTextCursor::KeepAnchor);
    editorWidget->setTextCursor(cursor);

    QTest::keyClick(editorWidget, Qt::Key_X);
    const QString edited = editorWidget->textDocument()->plainText();

    // the selection the redo has to bring back is not the one around the edit
    cursor = editorWidget->textCursor();
    cursor.setPosition(2);
    cursor.setPosition(6, QTextCursor::KeepAnchor);
    editorWidget->setTextCursor(cursor);

    editorWidget->undo();
    editorWidget->redo();

    QCOMPARE(editorWidget->textDocument()->plainText(), edited);
    cursor = editorWidget->textCursor();
    QVERIFY(cursor.hasSelection());
    QCOMPARE(cursor.anchor(), 2);
    QCOMPARE(cursor.position(), 6);
}

void UndoCursorTest::testSelectAllRestoredOnUndo()
{
    const QString contents = "first line\nsecond line";
    TextEditorWidget *editorWidget = openEditor(contents.toUtf8());
    QVERIFY(editorWidget);
    const QScopeGuard cleanup([this] { Core::EditorManager::closeEditors({m_editor}, false); });

    editorWidget->moveCursor(QTextCursor::End);
    editorWidget->selectAll();

    QTest::keyClick(editorWidget, Qt::Key_X);
    QCOMPARE(editorWidget->textDocument()->plainText(), QString("x"));

    editorWidget->undo();

    QCOMPARE(editorWidget->textDocument()->plainText(), contents);
    const QTextCursor cursor = editorWidget->textCursor();
    QCOMPARE(cursor.selectionStart(), 0);
    QCOMPARE(cursor.selectionEnd(), int(contents.size()));
}

void UndoCursorTest::testEditAfterUndo()
{
    const QString contents = "first line\nsecond line";
    TextEditorWidget *editorWidget = openEditor(contents.toUtf8());
    QVERIFY(editorWidget);
    const QScopeGuard cleanup([this] { Core::EditorManager::closeEditors({m_editor}, false); });

    editorWidget->moveCursor(QTextCursor::Start);
    QTest::keyClick(editorWidget, Qt::Key_A);
    editorWidget->undo();
    QCOMPARE(editorWidget->textDocument()->plainText(), contents);

    QTextCursor cursor = editorWidget->textCursor();
    cursor.setPosition(3);
    cursor.setPosition(7, QTextCursor::KeepAnchor);
    editorWidget->setTextCursor(cursor);
    QTest::keyClick(editorWidget, Qt::Key_B);
    QCOMPARE(editorWidget->textDocument()->plainText(), QString("firbine\nsecond line"));

    editorWidget->undo();

    QCOMPARE(editorWidget->textDocument()->plainText(), contents);
    cursor = editorWidget->textCursor();
    QCOMPARE(cursor.anchor(), 3);
    QCOMPARE(cursor.position(), 7);
}

void UndoCursorTest::testDropUndo()
{
    const QString contents = "foo bar baz";
    TextEditorWidget *editorWidget = openEditor(contents.toUtf8());
    QVERIFY(editorWidget);
    const QScopeGuard cleanup([this] { Core::EditorManager::closeEditors({m_editor}, false); });

    QTextCursor cursor = editorWidget->textCursor();
    cursor.setPosition(0);
    cursor.setPosition(3, QTextCursor::KeepAnchor);
    editorWidget->setTextCursor(cursor);

    QTextCursor end(editorWidget->document());
    end.movePosition(QTextCursor::End);
    QMimeData mimeData;
    mimeData.setText("foo");
    const QPoint dropPosition = editorWidget->cursorRect(end).center();
    // A drop goes to the widget that accepted the drag.
    QDragEnterEvent dragEnter(dropPosition, Qt::CopyAction, &mimeData, Qt::LeftButton, Qt::NoModifier);
    QCoreApplication::sendEvent(editorWidget->viewport(), &dragEnter);
    QVERIFY(dragEnter.isAccepted());
    QDropEvent drop(dropPosition, Qt::CopyAction, &mimeData, Qt::LeftButton, Qt::NoModifier);
    QCoreApplication::sendEvent(editorWidget->viewport(), &drop);
    QCOMPARE(editorWidget->textDocument()->plainText(), QString("foo bar bazfoo"));

    editorWidget->undo();

    QCOMPARE(editorWidget->textDocument()->plainText(), contents);
    cursor = editorWidget->textCursor();
    QCOMPARE(cursor.anchor(), 0);
    QCOMPARE(cursor.position(), 3);
}

void UndoCursorTest::testJoinedMoveLineUndo()
{
    TextEditorWidget *editorWidget = openEditor("a\nb\nc");
    QVERIFY(editorWidget);
    const QScopeGuard cleanup([this] { Core::EditorManager::closeEditors({m_editor}, false); });

    QTextCursor cursor = editorWidget->textCursor();
    cursor.setPosition(0);
    cursor.setPosition(1, QTextCursor::KeepAnchor);
    editorWidget->setTextCursor(cursor);
    QTest::keyClick(editorWidget, Qt::Key_X);

    cursor.setPosition(4);
    cursor.setPosition(5, QTextCursor::KeepAnchor);
    editorWidget->setTextCursor(cursor);
    editorWidget->moveLineUp();
    editorWidget->moveLineUp();
    QCOMPARE(editorWidget->textDocument()->plainText(), QString("c\nx\nb\n"));

    editorWidget->undo();

    QCOMPARE(editorWidget->textDocument()->plainText(), QString("x\nb\nc"));
    cursor = editorWidget->textCursor();
    QCOMPARE(cursor.anchor(), 4);
    QCOMPARE(cursor.position(), 5);

    editorWidget->undo();

    QCOMPARE(editorWidget->textDocument()->plainText(), QString("a\nb\nc"));
    cursor = editorWidget->textCursor();
    QCOMPARE(cursor.anchor(), 0);
    QCOMPARE(cursor.position(), 1);
}

void UndoCursorTest::testJoinedIndentUndo()
{
    TextEditorWidget *editorWidget = openEditor("a\nb\nc");
    QVERIFY(editorWidget);
    const QScopeGuard cleanup([this] { Core::EditorManager::closeEditors({m_editor}, false); });

    editorWidget->moveCursor(QTextCursor::End);
    QTest::keyClick(editorWidget, Qt::Key_X);
    const QString edited = editorWidget->textDocument()->plainText();
    QCOMPARE(edited, QString("a\nb\ncx"));

    QTextDocument *document = editorWidget->document();
    QTextCursor first(document);
    first.setPosition(0);
    first.setPosition(1, QTextCursor::KeepAnchor);
    QTextCursor second(document);
    second.setPosition(2);
    second.setPosition(3, QTextCursor::KeepAnchor);
    editorWidget->setMultiTextCursor(Utils::MultiTextCursor({first, second}));
    editorWidget->indent();
    QVERIFY(editorWidget->textDocument()->plainText() != edited);

    editorWidget->undo();

    QCOMPARE(editorWidget->textDocument()->plainText(), edited);
    const QList<QTextCursor> cursors = editorWidget->multiTextCursor().cursors();
    QCOMPARE(cursors.size(), 2);
    QCOMPARE(cursors.at(0).anchor(), 0);
    QCOMPARE(cursors.at(0).position(), 1);
    QCOMPARE(cursors.at(1).anchor(), 2);
    QCOMPARE(cursors.at(1).position(), 3);

    editorWidget->undo();

    QCOMPARE(editorWidget->textDocument()->plainText(), QString("a\nb\nc"));
    QVERIFY(!editorWidget->multiTextCursor().hasMultipleCursors());
    const QTextCursor cursor = editorWidget->textCursor();
    QVERIFY(!cursor.hasSelection());
    QCOMPARE(cursor.position(), 5);
}

void UndoCursorTest::testUndoRedoInDuplicate()
{
    QVERIFY(!Core::EditorManager::currentEditor());
    const QString contents = "first line\nsecond line";
    TextEditorWidget editorWidget;
    editorWidget.setTextDocument(TextDocumentPtr(new TextDocument));
    QVERIFY(editorWidget.textDocument()->setPlainText(contents));

    QTextCursor cursor = editorWidget.textCursor();
    cursor.setPosition(0);
    cursor.setPosition(5, QTextCursor::KeepAnchor);
    editorWidget.setTextCursor(cursor);
    QTest::keyClick(&editorWidget, Qt::Key_X);
    const QString firstEdit = editorWidget.textDocument()->plainText();
    QCOMPARE(firstEdit, QString("x line\nsecond line"));

    TextEditorWidget duplicate;
    duplicate.setTextDocument(editorWidget.textDocumentPtr());
    TextEditorWidget *duplicateWidget = &duplicate;

    cursor = duplicateWidget->textCursor();
    cursor.setPosition(7);
    cursor.setPosition(13, QTextCursor::KeepAnchor);
    duplicateWidget->setTextCursor(cursor);
    QTest::keyClick(duplicateWidget, Qt::Key_Y);
    const QString secondEdit = duplicateWidget->textDocument()->plainText();
    QCOMPARE(secondEdit, QString("x line\ny line"));

    duplicateWidget->undo();
    duplicateWidget->undo();
    QCOMPARE(duplicateWidget->textDocument()->plainText(), contents);

    duplicateWidget->redo();

    QCOMPARE(duplicateWidget->textDocument()->plainText(), firstEdit);
    cursor = duplicateWidget->textCursor();
    QCOMPARE(cursor.anchor(), 7);
    QCOMPARE(cursor.position(), 13);

    duplicateWidget->redo();

    QCOMPARE(duplicateWidget->textDocument()->plainText(), secondEdit);
    cursor = duplicateWidget->textCursor();
    QVERIFY(!cursor.hasSelection());
    QCOMPARE(cursor.position(), 8);
}

void UndoCursorTest::testUndoInOtherView()
{
    const QString contents = "first line\nsecond line";
    TextEditorWidget *editorWidget = openEditor(contents.toUtf8());
    QVERIFY(editorWidget);
    const QScopeGuard cleanup([this] { Core::EditorManager::closeEditors({m_editor}, false); });
    QCOMPARE(TextEditorWidget::currentTextEditorWidget(), editorWidget);

    const std::unique_ptr<BaseTextEditor> duplicate(
        qobject_cast<BaseTextEditor *>(m_editor)->duplicate());
    QVERIFY(duplicate);
    TextEditorWidget *duplicateWidget = duplicate->editorWidget();
    QTextCursor cursor = duplicateWidget->textCursor();
    cursor.setPosition(15);
    duplicateWidget->setTextCursor(cursor);

    cursor = editorWidget->textCursor();
    cursor.setPosition(0);
    cursor.setPosition(5, QTextCursor::KeepAnchor);
    editorWidget->setTextCursor(cursor);
    QTest::keyClick(editorWidget, Qt::Key_X);

    duplicateWidget->undo();

    // The undo in the other view moves its cursor to the undone edit.
    QCOMPARE(duplicateWidget->textDocument()->plainText(), contents);
    cursor = duplicateWidget->textCursor();
    QVERIFY(!cursor.hasSelection());
    QCOMPARE(cursor.position(), 5);
}

void UndoCursorTest::testUndoOfEditInOtherEditor()
{
    const QString contents = "first line\nsecond line";
    TextEditorWidget *otherWidget = openEditor(contents.toUtf8());
    QVERIFY(otherWidget);
    Core::IEditor *otherEditor = m_editor;
    const QScopeGuard closeOther([otherEditor] {
        Core::EditorManager::closeEditors({otherEditor}, false);
    });
    TextEditorWidget *editorWidget = openEditor("current");
    QVERIFY(editorWidget);
    const QScopeGuard cleanup([this] { Core::EditorManager::closeEditors({m_editor}, false); });
    QCOMPARE(TextEditorWidget::currentTextEditorWidget(), editorWidget);

    // Like a refactoring that changes another open file.
    QTextCursor edit(otherWidget->document());
    edit.setPosition(11);
    edit.insertText("x");
    QCOMPARE(otherWidget->textDocument()->plainText(), QString("first line\nxsecond line"));

    otherWidget->undo();

    QCOMPARE(otherWidget->textDocument()->plainText(), contents);
    const QTextCursor cursor = otherWidget->textCursor();
    QVERIFY(!cursor.hasSelection());
    QCOMPARE(cursor.position(), 11);
}

class NonTextDocument final : public Core::IDocument
{
public:
    NonTextDocument() { setId("TextEditor.Test.NonTextEditor"); }

    Utils::Result<> setContents(const QByteArray &) final { return Utils::ResultOk; }
};

// Like the Android manifest editor, which embeds a text editor in a widget of its own.
class NonTextEditor final : public Core::IEditor
{
public:
    NonTextEditor()
    {
        setWidget(m_widget.get());
        m_embeddedWidget->setTextDocument(TextDocumentPtr(new TextDocument));
    }

    Core::IDocument *document() const final { return m_document.get(); }
    QWidget *toolBar() final { return nullptr; }
    TextEditorWidget *embeddedWidget() const { return m_embeddedWidget; }

private:
    const std::unique_ptr<NonTextDocument> m_document = std::make_unique<NonTextDocument>();
    const std::unique_ptr<QWidget> m_widget = std::make_unique<QWidget>();
    TextEditorWidget *const m_embeddedWidget = new TextEditorWidget(m_widget.get());
};

class NonTextEditorFactory final : public Core::IEditorFactory
{
public:
    NonTextEditorFactory()
    {
        setId("TextEditor.Test.NonTextEditor");
        setDisplayName("Non-Text Editor");
        setEditorCreator([] { return new NonTextEditor; });
    }
};

void UndoCursorTest::testUndoOfEditWhileNonTextEditorIsCurrent()
{
    const QString contents = "first line\nsecond line";
    TextEditorWidget *editorWidget = openEditor(contents.toUtf8());
    QVERIFY(editorWidget);
    const QScopeGuard cleanup([this] { Core::EditorManager::closeEditors({m_editor}, false); });

    const NonTextEditorFactory factory;
    QString title = "non_text";
    Core::IEditor *nonTextEditor
        = Core::EditorManager::openEditorWithContents(factory.id(), &title, {});
    QVERIFY(nonTextEditor);
    const QScopeGuard closeNonText([nonTextEditor] {
        Core::EditorManager::closeEditors({nonTextEditor}, false);
    });
    QCOMPARE(Core::EditorManager::currentEditor(), nonTextEditor);
    QCOMPARE(TextEditorWidget::currentTextEditorWidget(), nullptr);

    // Like a Replace All in open files while a form or diff editor is current.
    QTextCursor edit(editorWidget->document());
    edit.setPosition(11);
    edit.insertText("x");
    QCOMPARE(editorWidget->textDocument()->plainText(), QString("first line\nxsecond line"));

    editorWidget->undo();

    QCOMPARE(editorWidget->textDocument()->plainText(), contents);
    const QTextCursor cursor = editorWidget->textCursor();
    QVERIFY(!cursor.hasSelection());
    QCOMPARE(cursor.position(), 11);
}

void UndoCursorTest::testUndoInEmbeddedWidget()
{
    const NonTextEditorFactory factory;
    QString title = "non_text";
    Core::IEditor *editor = Core::EditorManager::openEditorWithContents(factory.id(), &title, {});
    QVERIFY(editor);
    const QScopeGuard cleanup([editor] { Core::EditorManager::closeEditors({editor}, false); });
    QCOMPARE(Core::EditorManager::currentEditor(), editor);

    TextEditorWidget *editorWidget = static_cast<NonTextEditor *>(editor)->embeddedWidget();
    const QString contents = "first line\nsecond line";
    QVERIFY(editorWidget->textDocument()->setPlainText(contents));
    QTextCursor cursor = editorWidget->textCursor();
    cursor.setPosition(0);
    cursor.setPosition(5, QTextCursor::KeepAnchor);
    editorWidget->setTextCursor(cursor);
    QVERIFY(!editorWidget->hasFocus());

    QTest::keyClick(editorWidget, Qt::Key_X);
    QCOMPARE(editorWidget->textDocument()->plainText(), QString("x line\nsecond line"));

    editorWidget->undo();

    QCOMPARE(editorWidget->textDocument()->plainText(), contents);
    cursor = editorWidget->textCursor();
    QCOMPARE(cursor.anchor(), 0);
    QCOMPARE(cursor.position(), 5);
}

void UndoCursorTest::testUndoInFocusedWidget()
{
    TextEditorWidget *currentWidget = openEditor("current");
    QVERIFY(currentWidget);
    const QScopeGuard cleanup([this] { Core::EditorManager::closeEditors({m_editor}, false); });

    // Like the text editor of a settings page or of another mode.
    TextEditorWidget editorWidget;
    editorWidget.setTextDocument(TextDocumentPtr(new TextDocument));
    const QString contents = "first line\nsecond line";
    QVERIFY(editorWidget.textDocument()->setPlainText(contents));
    editorWidget.show();
    editorWidget.activateWindow();
    if (!QTest::qWaitForWindowActive(&editorWidget))
        QSKIP("The window of the text editor does not become active.");
    editorWidget.setFocus();
    QVERIFY(editorWidget.hasFocus());
    QCOMPARE(TextEditorWidget::currentTextEditorWidget(), currentWidget);

    QTextCursor cursor = editorWidget.textCursor();
    cursor.setPosition(0);
    cursor.setPosition(5, QTextCursor::KeepAnchor);
    editorWidget.setTextCursor(cursor);
    QTest::keyClick(&editorWidget, Qt::Key_X);
    QCOMPARE(editorWidget.textDocument()->plainText(), QString("x line\nsecond line"));

    editorWidget.undo();

    QCOMPARE(editorWidget.textDocument()->plainText(), contents);
    cursor = editorWidget.textCursor();
    QCOMPARE(cursor.anchor(), 0);
    QCOMPARE(cursor.position(), 5);
}

QObject *createUndoCursorTest()
{
    return new UndoCursorTest;
}

// Regression test for QTCREATORBUG-31149: reflowing a comment must not pull
// adjacent code lines into the paragraph.
class RewrapParagraphTest final : public QObject
{
    Q_OBJECT

private slots:
    void testRewrapParagraph_data();
    void testRewrapParagraph();
    void testRewrapSelection_data();
    void testRewrapSelection();
};

void RewrapParagraphTest::testRewrapParagraph_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<int>("cursorBlock");
    QTest::addColumn<QString>("expected");

    // A short single-line comment surrounded by code stays untouched; the
    // code lines must not be merged into it.
    QTest::newRow("singleLineCommentAmongCode")
        << "int a = 1;\n// hello world\nint b = 2;" << 1
        << "int a = 1;\n// hello world\nint b = 2;";

    // Consecutive "//" comment lines reflow together, but the surrounding
    // code is left alone.
    QTest::newRow("multiLineCommentAmongCode")
        << "int a = 1;\n// alpha beta\n// gamma delta\nint b = 2;" << 1
        << "int a = 1;\n// alpha beta gamma delta\nint b = 2;";

    // Plain-text paragraphs are still bounded by blank lines only.
    QTest::newRow("plainTextParagraph")
        << "one two\nthree four\n\nnext" << 0
        << "one two three four\n\nnext";

    // Doxygen "///" comments keep their leader when reflowed.
    QTest::newRow("doxygenComment")
        << "/// alpha beta\n/// gamma delta\n" << 0
        << "/// alpha beta gamma delta\n";

    // A single "//" comment line too long for the margin wraps into several
    // lines that each keep the "//" leader (QTCREATORBUG-5674).
    QTest::newRow("singleLineCommentWraps")
        << "// aaaaaaaaa aaaaaaaaa aaaaaaaaa aaaaaaaaa aaaaaaaaa aaaaaaaaa "
           "aaaaaaaaa aaaaaaaaa\n" << 0
        << "// aaaaaaaaa aaaaaaaaa aaaaaaaaa aaaaaaaaa aaaaaaaaa aaaaaaaaa "
           "aaaaaaaaa\n// aaaaaaaaa\n";

    // In a "/** */" block the line following the paragraph is a bare "*" or
    // the closing "*/", neither of which shares the space that separates the
    // leader from the text. That space belongs to the leader nonetheless: a
    // paragraph that needs no wrapping comes back unchanged.
    QTest::newRow("doxygenBlockKeepsLeaderSpace")
        << "/**\n * @brief alpha beta\n */\n" << 1
        << "/**\n * @brief alpha beta\n */\n";

    // The separator is taken from the line as it is, so a tab survives as one.
    QTest::newRow("doxygenBlockKeepsLeaderTab")
        << "/**\n *\t@brief alpha beta\n */\n" << 1
        << "/**\n *\t@brief alpha beta\n */\n";

    // And one that does need wrapping keeps the leader on the new line.
    QTest::newRow("doxygenBlockWraps")
        << "/**\n  * @brief function is a function that calculates some result "
           "from the parameter d, but does not return the result\n  *\n"
           "  * @param d is the parameter that is used in calculation.\n  */\n" << 1
        << "/**\n  * @brief function is a function that calculates some result "
           "from the\n  * parameter d, but does not return the result\n  *\n"
           "  * @param d is the parameter that is used in calculation.\n  */\n";

    // The recovered space is written out, but not required of the other lines:
    // a continuation line without it must not leave its leader behind as text.
    QTest::newRow("doxygenBlockMixedLeaderSpacing")
        << "  * alpha beta\n  *gamma delta\n" << 0
        << "  * alpha beta gamma delta\n";

    // Continuation lines aligned under the text of a doxygen command keep that
    // alignment; without this the indent is collapsed into a single space and
    // the paragraph comes back flush with the leader.
    QTest::newRow("doxygenBlockKeepsHangingIndent")
        << "/**\n  * @brief function is a function that calculates some result\n"
           "  *        from the parameter d, but does not return the result\n  *\n"
           "  * @param d is the parameter that is used in calculation.\n  */\n" << 1
        << "/**\n  * @brief function is a function that calculates some result "
           "from the\n  *        parameter d, but does not return the result\n  *\n"
           "  * @param d is the parameter that is used in calculation.\n  */\n";

    // Trailing whitespace on the continuation line does not belong to the
    // indent, and must not be mistaken for part of it.
    QTest::newRow("doxygenBlockHangingIndentTrailingSpace")
        << "/**\n  * @brief function is a function that calculates some result\n"
           "  *        from the parameter d, but does not return the result   \n  *\n"
           "  * @param d is the parameter that is used in calculation.\n  */\n" << 1
        << "/**\n  * @brief function is a function that calculates some result "
           "from the\n  *        parameter d, but does not return the result\n  *\n"
           "  * @param d is the parameter that is used in calculation.\n  */\n";

    // A command still on one line takes the indent from the command above it,
    // so reflowing it follows the alignment the block already uses.
    QTest::newRow("hangingIndentReusedWithinBlock")
        << "/**\n  * @brief alpha beta\n  *        gamma delta\n  *\n"
           "  * @param d is the parameter that is used in calculation and here "
           "is more text to force a wrap\n  */\n" << 4
        << "/**\n  * @brief alpha beta\n  *        gamma delta\n  *\n"
           "  * @param d is the parameter that is used in calculation and here "
           "is more text\n  *        to force a wrap\n  */\n";

    // An indent from a different comment block is none of this block's
    // business, so it is not carried across.
    QTest::newRow("hangingIndentNotReusedAcrossBlocks")
        << "/**\n  * @brief alpha beta\n  *        gamma delta\n  */\nint x;\n"
           "/**\n  * @param d is the parameter that is used in calculation and "
           "here is more text to force a wrap\n  */\n" << 6
        << "/**\n  * @brief alpha beta\n  *        gamma delta\n  */\nint x;\n"
           "/**\n  * @param d is the parameter that is used in calculation and "
           "here is more text\n  * to force a wrap\n  */\n";

    // A paragraph whose continuation lines are flush with the leader is laid
    // out that way deliberately, and keeps that layout even where the command
    // above it is aligned.
    QTest::newRow("hangingIndentNotReusedForFlushParagraph")
        << "/**\n  * @brief alpha beta\n  *        gamma delta\n  *\n"
           "  * @param d is the parameter that is used in calculation and here "
           "is more text to force a wrap\n  * bar baz\n  */\n" << 4
        << "/**\n  * @brief alpha beta\n  *        gamma delta\n  *\n"
           "  * @param d is the parameter that is used in calculation and here "
           "is more text\n  * to force a wrap bar baz\n  */\n";

    // The same holds for a run of C++ style comment lines.
    QTest::newRow("lineCommentHangingIndentReused")
        << "/// @brief alpha beta\n///        gamma delta\n///\n"
           "/// @param d is the parameter that is used in calculation and here "
           "is more text to force a wrap\n" << 3
        << "/// @brief alpha beta\n///        gamma delta\n///\n"
           "/// @param d is the parameter that is used in calculation and here "
           "is more text\n///        to force a wrap\n";

    // Outside a doxygen command an indented second line has no agreed meaning,
    // so it is left to be reflowed away as before.
    QTest::newRow("hangingIndentOnlyUnderDoxygenCommand")
        << "  * aaaaaaaaa aaaaaaaaa aaaaaaaaa aaaaaaaaa aaaaaaaaa aaaaaaaaa "
           "aaaaaaaaa\n  *     bbbbbbbbb\n" << 0
        << "  * aaaaaaaaa aaaaaaaaa aaaaaaaaa aaaaaaaaa aaaaaaaaa aaaaaaaaa "
           "aaaaaaaaa\n  * bbbbbbbbb\n";
}

void RewrapParagraphTest::testRewrapParagraph()
{
    QFETCH(QString, input);
    QFETCH(int, cursorBlock);
    QFETCH(QString, expected);

    QString title = "rewrap.txt";
    Core::IEditor *editor = Core::EditorManager::openEditorWithContents(
        Core::Constants::K_DEFAULT_TEXT_EDITOR_ID, &title, input.toUtf8());
    QVERIFY(editor);
    auto baseEditor = qobject_cast<BaseTextEditor *>(editor);
    QVERIFY(baseEditor);
    TextEditorWidget *editorWidget = baseEditor->editorWidget();
    QVERIFY(editorWidget);

    MarginSettingsData margin = editorWidget->marginSettings();
    margin.m_marginColumn = 80;
    editorWidget->setMarginSettings(margin);

    QTextCursor cursor = editorWidget->textCursor();
    cursor.movePosition(QTextCursor::Start);
    for (int i = 0; i < cursorBlock; ++i)
        cursor.movePosition(QTextCursor::NextBlock);
    editorWidget->setTextCursor(cursor);

    editorWidget->rewrapParagraph();

    QCOMPARE(editorWidget->textDocument()->plainText(), expected);

    Core::EditorManager::closeEditors({editor}, false);
}

void RewrapParagraphTest::testRewrapSelection_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<int>("anchorBlock");
    QTest::addColumn<int>("cursorBlock");
    QTest::addColumn<QString>("expected");

    // Selecting a whole comment reflows all of its paragraphs, whichever end
    // of the selection the cursor is at, and leaves the blank comment line
    // that separates them alone.
    QTest::newRow("wholeCommentWithCursorAtTheEnd")
        << "/**\n  * @brief function is a function that calculates some result "
           "from the parameter d, but does not return the result\n  *\n"
           "  * @param d is the parameter that is used in the calculation of "
           "the result that is returned\n  */\n" << 0 << 4
        << "/**\n  * @brief function is a function that calculates some result "
           "from the\n  * parameter d, but does not return the result\n  *\n"
           "  * @param d is the parameter that is used in the calculation of "
           "the result\n  * that is returned\n  */\n";

    // Plain-text paragraphs delimited by a blank line are reflowed one by one,
    // and each of them as a whole even where the selection covers only part of
    // it: the paragraph, not the selection, is the unit.
    QTest::newRow("selectionOverSeveralPlainTextParagraphs")
        << "one two\nthree four\n\nfive six\nseven eight\n" << 1 << 4
        << "one two three four\n\nfive six seven eight\n";

    // Code and a run of "//" comment lines are never one paragraph, so a
    // selection covering both reflows the comment without absorbing the code.
    QTest::newRow("selectionOverCodeAndComment")
        << "int a = 1;\n// alpha beta\n// gamma delta\nint b = 2;\n" << 0 << 4
        << "int a = 1;\n// alpha beta gamma delta\nint b = 2;\n";

    // A run of plain statements carries no signal - no blank line, no comment
    // leader - that tells it apart from a run of prose lines, so a selection
    // over it reflows the same way a plain-text paragraph does: this is not a
    // new consequence of acting on a selection, it already happened with a
    // single cursor placed in the middle of such a run.
    QTest::newRow("selectionOverPlainCodeStatements")
        << "int a = 1;\nint b = 2;\n" << 0 << 2
        << "int a = 1; int b = 2;\n";
}

void RewrapParagraphTest::testRewrapSelection()
{
    QFETCH(QString, input);
    QFETCH(int, anchorBlock);
    QFETCH(int, cursorBlock);
    QFETCH(QString, expected);

    QString title = "rewrap.txt";
    Core::IEditor *editor = Core::EditorManager::openEditorWithContents(
        Core::Constants::K_DEFAULT_TEXT_EDITOR_ID, &title, input.toUtf8());
    QVERIFY(editor);
    auto baseEditor = qobject_cast<BaseTextEditor *>(editor);
    QVERIFY(baseEditor);
    TextEditorWidget *editorWidget = baseEditor->editorWidget();
    QVERIFY(editorWidget);

    MarginSettingsData margin = editorWidget->marginSettings();
    margin.m_marginColumn = 80;
    editorWidget->setMarginSettings(margin);

    QTextCursor cursor = editorWidget->textCursor();
    cursor.movePosition(QTextCursor::Start);
    for (int i = 0; i < anchorBlock; ++i)
        cursor.movePosition(QTextCursor::NextBlock);
    for (int i = anchorBlock; i < cursorBlock; ++i)
        cursor.movePosition(QTextCursor::NextBlock, QTextCursor::KeepAnchor);
    if (anchorBlock == cursorBlock)
        cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
    QVERIFY(cursor.hasSelection());
    editorWidget->setTextCursor(cursor);

    editorWidget->rewrapParagraph();

    QCOMPARE(editorWidget->textDocument()->plainText(), expected);

    Core::EditorManager::closeEditors({editor}, false);
}

QObject *createRewrapParagraphTest()
{
    return new RewrapParagraphTest;
}

class ExternalToolReadOnlyTest final : public QObject
{
    Q_OBJECT

private slots:
    void testReadOnlyEditorNotModified()
    {
        QString title = "readonly.txt";
        Core::IEditor *editor = Core::EditorManager::openEditorWithContents(
            Core::Constants::K_DEFAULT_TEXT_EDITOR_ID, &title, "b\na\n");
        QVERIFY(editor);
        const QScopeGuard cleanup([&] { Core::EditorManager::closeEditors({editor}, false); });
        auto baseEditor = qobject_cast<BaseTextEditor *>(editor);
        QVERIFY(baseEditor);
        TextEditorWidget *editorWidget = baseEditor->editorWidget();
        QVERIFY(editorWidget);

        editorWidget->setReadOnly(true);
        Core::ExternalToolManager::emitReplaceSelectionRequested("sorted");
        QCOMPARE(baseEditor->textDocument()->plainText(), QString("b\na\n"));

        editorWidget->setReadOnly(false);
        Core::ExternalToolManager::emitReplaceSelectionRequested("sorted");
        QVERIFY(baseEditor->textDocument()->plainText().contains("sorted"));
    }
};

QObject *createExternalToolReadOnlyTest()
{
    return new ExternalToolReadOnlyTest;
}

class RevertToSavedTest final : public QObject
{
    Q_OBJECT

private slots:
    void testRevertToSaved();
};

void RevertToSavedTest::testRevertToSaved()
{
    Utils::TemporaryDirectory tempDir("qtc-revert-test");
    QVERIFY(tempDir.isValid());
    const Utils::FilePath file = tempDir.path().pathAppended("revert.txt");
    const QString original = "line one\nline two\nline three";
    QVERIFY(file.writeFileContents(original.toUtf8()).has_value());

    Core::IEditor *editor = Core::EditorManager::openEditor(file);
    QVERIFY(editor);
    auto document = qobject_cast<TextDocument *>(editor->document());
    QVERIFY(document);
    QVERIFY(!document->isModified()); // "Revert to Saved" is disabled while unmodified

    QTextCursor cursor(document->document());
    cursor.insertText("modified ");
    QVERIFY(document->isModified());

    // The non-interactive core of the "Revert to Saved" action.
    const Utils::Result<> result = document->reload(Core::IDocument::FlagReload,
                                                    Core::IDocument::TypeContents);
    QVERIFY(result.has_value());
    QVERIFY(!document->isModified());
    QCOMPARE(document->plainText(), original);

    Core::EditorManager::closeEditors({editor}, false);
}

QObject *createRevertToSavedTest()
{
    return new RevertToSavedTest;
}

class FindReplaceTest final : public QObject
{
    Q_OBJECT

private slots:
    void testReplaceAll();
    void testReplaceSelected();
    void testFindAcrossLines();
    void testFindAcrossLinesBackward();
    void testFindLiteralNewline();
    void testReplaceAllAcrossLines();
    void testReplaceAllAcrossLinesInScope();
    void testFindScopeOnOneLine();
    void testSelectionDoesNotLimitTheSearch();
    void testFindInSelectionCommand();
    void testCurrentFindString();
    void testReplaceAllAcrossLinesAdjacent();
    void testAllMatchesAcrossBlocks();
    void testHighlightAcrossLines();
    void testHighlightFollowsTheSearch();
    void testMultiLineResultReplace();
    void testMultiLineResultSelectedOnActivation();
};

static BaseTextEditor *openTextEditor(const QString &contents, QString title)
{
    Core::IEditor *editor = Core::EditorManager::openEditorWithContents(
        Core::Constants::K_DEFAULT_TEXT_EDITOR_ID, &title, contents.toUtf8());
    return qobject_cast<BaseTextEditor *>(editor);
}

void FindReplaceTest::testReplaceAll()
{
    const QString content = "int window;\nWindow w;\nTriangleWindow tw;";
    BaseTextEditor *editor = openTextEditor(content, "find.txt");
    QVERIFY(editor);
    TextEditorWidget *editorWidget = editor->editorWidget();

    // Case-insensitive by default, so every "window"/"Window" is replaced.
    Core::BaseTextFind find(editorWidget);
    const int count = find.replaceAll("window", "find", {});

    QCOMPARE(count, 3);
    QCOMPARE(editorWidget->textDocument()->plainText(),
             QString("int find;\nfind w;\nTrianglefind tw;"));

    Core::EditorManager::closeEditors({editor}, false);
}

void FindReplaceTest::testReplaceSelected()
{
    const QString content = "int window;\nWindow w;\nTriangleWindow tw;";
    BaseTextEditor *editor = openTextEditor(content, "find.txt");
    QVERIFY(editor);
    TextEditorWidget *editorWidget = editor->editorWidget();

    // Select the "Window" on the second line and replace only that occurrence.
    QTextCursor cursor(editorWidget->document());
    cursor.setPosition(12);
    cursor.setPosition(18, QTextCursor::KeepAnchor);
    QCOMPARE(cursor.selectedText(), QString("Window"));
    editorWidget->setTextCursor(cursor);

    Core::BaseTextFind find(editorWidget);
    find.replace("Window", "find", {});

    QCOMPARE(editorWidget->textDocument()->plainText(),
             QString("int window;\nfind w;\nTriangleWindow tw;"));

    Core::EditorManager::closeEditors({editor}, false);
}

void FindReplaceTest::testFindAcrossLines()
{
    BaseTextEditor *editor = openTextEditor("int a;\nint b;\nint c;", "find.txt");
    QVERIFY(editor);
    TextEditorWidget *editorWidget = editor->editorWidget();

    Core::BaseTextFind find(editorWidget);
    QCOMPARE(find.findStep("a;\\nint b", Utils::FindRegularExpression), Core::IFindSupport::Found);
    QCOMPARE(editorWidget->textCursor().selectedText(),
             QString("a;") + QChar(QChar::ParagraphSeparator) + "int b");

    // CRLF is how the user spells a line break, the document only has LF
    editorWidget->moveCursor(QTextCursor::Start);
    QCOMPARE(find.findStep("b;\\r\\nint", Utils::FindRegularExpression),
             Core::IFindSupport::Found);
    QCOMPARE(editorWidget->textCursor().selectionStart(), 11);

    QCOMPARE(find.findStep("a;\\nint b", Utils::FindRegularExpression),
             Core::IFindSupport::Found); // wraps
    QCOMPARE(find.findStep("a;\\nint b", Utils::FindRegularExpression | Utils::FindWholeWords),
             Core::IFindSupport::Found);
    QCOMPARE(find.findStep("a;\\nxint", Utils::FindRegularExpression),
             Core::IFindSupport::NotFound);

    Core::EditorManager::closeEditors({editor}, false);
}

void FindReplaceTest::testFindAcrossLinesBackward()
{
    BaseTextEditor *editor = openTextEditor("x\ny\nx\ny", "find.txt");
    QVERIFY(editor);
    TextEditorWidget *editorWidget = editor->editorWidget();
    editorWidget->moveCursor(QTextCursor::End);

    Core::BaseTextFind find(editorWidget);
    const Utils::FindFlags flags = Utils::FindRegularExpression | Utils::FindBackward;
    QCOMPARE(find.findStep("x\\ny", flags), Core::IFindSupport::Found);
    QCOMPARE(editorWidget->textCursor().selectionStart(), 4);
    QCOMPARE(find.findStep("x\\ny", flags), Core::IFindSupport::Found);
    QCOMPARE(editorWidget->textCursor().selectionStart(), 0);

    Core::EditorManager::closeEditors({editor}, false);
}

void FindReplaceTest::testFindLiteralNewline()
{
    BaseTextEditor *editor = openTextEditor("a.b\nc\nab\nc", "find.txt");
    QVERIFY(editor);
    TextEditorWidget *editorWidget = editor->editorWidget();

    // Not a regular expression: the dot is a dot and the newline is a newline
    Core::BaseTextFind find(editorWidget);
    QCOMPARE(find.findStep("a.b\nc", {}), Core::IFindSupport::Found);
    QCOMPARE(editorWidget->textCursor().selectionStart(), 0);
    QCOMPARE(find.findStep("ab\nc", {}), Core::IFindSupport::Found);
    QCOMPARE(editorWidget->textCursor().selectionStart(), 6);

    Core::EditorManager::closeEditors({editor}, false);
}

void FindReplaceTest::testReplaceAllAcrossLines()
{
    BaseTextEditor *editor = openTextEditor("a,\nb,\nc", "find.txt");
    QVERIFY(editor);
    TextEditorWidget *editorWidget = editor->editorWidget();

    Core::BaseTextFind find(editorWidget);
    QCOMPARE(find.replaceAll(",\\n(\\w)", " \\1", Utils::FindRegularExpression), 2);
    QCOMPARE(editorWidget->textDocument()->plainText(), QString("a b c"));

    Core::EditorManager::closeEditors({editor}, false);
}

static Utils::SearchResultItem multiLineResult(const Utils::FilePath &file)
{
    Utils::SearchResultItem item;
    item.setFilePath(file);
    item.setMainRange(Utils::Text::Range{{2, 1}, {3, 2}});
    item.setLineText("bXX\ncYY");
    item.setUserData(QStringList());
    return item;
}

void FindReplaceTest::testMultiLineResultReplace()
{
    const Utils::FilePath file = Utils::TemporaryDirectory::masterDirectoryFilePath() / "multiline.txt";
    QVERIFY(file.writeFileContents("a\nbXX\ncYY\nd\n"));
    const QScopeGuard cleanup([file] { file.removeFile(); });

    const Utils::FilePaths changed = BaseFileFind::replaceAll("--", {multiLineResult(file)}, false);
    QCOMPARE(changed, Utils::FilePaths{file});
    const Utils::Result<QByteArray> contents = file.fileContents();
    QVERIFY(contents);
    QCOMPARE(QString::fromUtf8(*contents), QString("a\nb--Y\nd\n"));
}

void FindReplaceTest::testMultiLineResultSelectedOnActivation()
{
    const Utils::FilePath file = Utils::TemporaryDirectory::masterDirectoryFilePath() / "multiline2.txt";
    QVERIFY(file.writeFileContents("a\nbXX\ncYY\nd\n"));
    const QScopeGuard cleanup([file] { file.removeFile(); });

    Core::EditorManager::openEditorAtSearchResult(multiLineResult(file));
    Core::IEditor *editor = Core::EditorManager::currentEditor();
    QVERIFY(editor);
    const QScopeGuard closeEditor([editor] { Core::EditorManager::closeEditors({editor}, false); });
    auto *textEdit = Utils::Aggregation::query<Utils::PlainTextEdit>(editor->widget());
    QVERIFY(textEdit);
    QCOMPARE(textEdit->textCursor().selectedText().replace(QChar::ParagraphSeparator, '\n'),
             QString("XX\ncY"));
}

void FindReplaceTest::testAllMatchesAcrossBlocks()
{
    const QString text = "x\ny\nx\ny\nxy";
    const QRegularExpression expr = Core::BaseTextFindBase::regularExpression(
        "x\\ny", Utils::FindRegularExpression);
    QVERIFY(Core::BaseTextFindBase::spansLines(expr));
    const QList<std::pair<int, int>> expected{{0, 3}, {4, 7}};
    QCOMPARE(Core::BaseTextFindBase::allMatchesAcrossBlocks(text, expr, false), expected);

    const QRegularExpression single = Core::BaseTextFindBase::regularExpression(
        "xy", Utils::FindRegularExpression);
    QVERIFY(!Core::BaseTextFindBase::spansLines(single));
}

void FindReplaceTest::testHighlightAcrossLines()
{
    BaseTextEditor *editor = openTextEditor("aaa\nbbb\nccc", "find.txt");
    QVERIFY(editor);
    TextEditorWidget *editorWidget = editor->editorWidget();
    editorWidget->resize(400, 200);
    editorWidget->show();

    auto *find = Utils::Aggregation::query<Core::IFindSupport>(editorWidget);
    QVERIFY(find);

    const auto renderedRows = [&](const QString &pattern) {
        find->highlightAll(pattern, Utils::FindRegularExpression);
        return editorWidget->viewport()->grab().toImage();
    };
    const QImage plain = renderedRows(QString());
    const QImage highlighted = renderedRows("a\\nbb");

    const auto differsInRow = [&](int y) {
        for (int x = 0; x < plain.width(); ++x) {
            if (plain.pixel(x, y) != highlighted.pixel(x, y))
                return true;
        }
        return false;
    };
    const auto rowOf = [&](int position) {
        QTextCursor cursor(editorWidget->document());
        cursor.setPosition(position);
        return editorWidget->cursorRect(cursor).center().y();
    };
    QVERIFY(differsInRow(rowOf(2)));  // the "a" on the first line
    QVERIFY(differsInRow(rowOf(5)));  // the "bb" on the second line
    QVERIFY(!differsInRow(rowOf(9))); // nothing on the third line

    Core::EditorManager::closeEditors({editor}, false);
}

void FindReplaceTest::testHighlightFollowsTheSearch()
{
    BaseTextEditor *editor = openTextEditor("aaa\nbbb\nccc\nddd", "find.txt");
    QVERIFY(editor);
    TextEditorWidget *editorWidget = editor->editorWidget();
    editorWidget->resize(400, 200);
    editorWidget->show();

    auto *find = Utils::Aggregation::query<Core::IFindSupport>(editorWidget);
    QVERIFY(find);

    const auto render = [&](const QString &pattern) {
        find->highlightAll(pattern, Utils::FindRegularExpression);
        return editorWidget->viewport()->grab().toImage();
    };
    const auto differsInRow = [&](const QImage &a, const QImage &b, int position) {
        QTextCursor cursor(editorWidget->document());
        cursor.setPosition(position);
        const int y = editorWidget->cursorRect(cursor).center().y();
        for (int x = 0; x < a.width(); ++x) {
            if (a.pixel(x, y) != b.pixel(x, y))
                return true;
        }
        return false;
    };

    const QImage plain = render(QString());
    render("a\\nb");
    const QImage second = render("c\\nd"); // another search over the same document

    QVERIFY(differsInRow(plain, second, 9));  // the "c"
    QVERIFY(!differsInRow(plain, second, 2)); // the "a" of the previous search

    Core::EditorManager::closeEditors({editor}, false);
}

void FindReplaceTest::testReplaceAllAcrossLinesInScope()
{
    BaseTextEditor *editor = openTextEditor("a\nb\na\nb\na\nb", "find.txt");
    QVERIFY(editor);
    TextEditorWidget *editorWidget = editor->editorWidget();

    // Only the lines 3 and 4 are in the scope of the search
    QTextCursor cursor(editorWidget->document());
    cursor.setPosition(4);
    cursor.setPosition(8, QTextCursor::KeepAnchor);
    editorWidget->setTextCursor(cursor);
    Core::BaseTextFind<TextEditorWidget> find(editorWidget);
    find.defineFindScope();

    QCOMPARE(find.replaceAll("a\\nb", "X", Utils::FindRegularExpression), 1);
    QCOMPARE(editorWidget->textDocument()->plainText(), QString("a\nb\nX\na\nb"));

    Core::EditorManager::closeEditors({editor}, false);
}

void FindReplaceTest::testFindScopeOnOneLine()
{
    BaseTextEditor *editor = openTextEditor("a a\na a", "find.txt");
    QVERIFY(editor);
    TextEditorWidget *editorWidget = editor->editorWidget();

    // Only the second line is in the scope of the search
    QTextCursor cursor(editorWidget->document());
    cursor.setPosition(4);
    cursor.setPosition(7, QTextCursor::KeepAnchor);
    editorWidget->setTextCursor(cursor);
    Core::BaseTextFind<TextEditorWidget> find(editorWidget);
    find.defineFindScope();

    QCOMPARE(find.replaceAll("a", "X", {}), 2);
    QCOMPARE(editorWidget->textDocument()->plainText(), QString("a a\nX X"));

    Core::EditorManager::closeEditors({editor}, false);
}

static void selectLinesThreeAndFour(TextEditorWidget *editorWidget)
{
    QTextCursor cursor(editorWidget->document());
    cursor.setPosition(4);
    cursor.setPosition(8, QTextCursor::KeepAnchor);
    editorWidget->setTextCursor(cursor);
}

// A widget only gets the focus while its window is active
static bool activateMainWindow()
{
    Core::ICore::raiseWindow(Core::ICore::mainWindow());
    return QTest::qWaitFor([] { return QApplication::activeWindow() == Core::ICore::mainWindow(); });
}

// The find tool bar acts on the widget with the focus, which it learns about through a queued
// connection
static bool focusForFind(TextEditorWidget *editorWidget)
{
    editorWidget->setFocus();
    if (!QTest::qWaitFor([editorWidget] { return editorWidget->hasFocus(); }))
        return false;
    bool focusChangeSeen = false;
    QMetaObject::invokeMethod(
        qApp, [&focusChangeSeen] { focusChangeSeen = true; }, Qt::QueuedConnection);
    return QTest::qWaitFor([&focusChangeSeen] { return focusChangeSeen; });
}

void FindReplaceTest::testSelectionDoesNotLimitTheSearch()
{
    if (!activateMainWindow())
        QSKIP("The main window does not become active.");
    BaseTextEditor *editor = openTextEditor("a\nb\na\nb\na\nb", "find.txt");
    QVERIFY(editor);
    TextEditorWidget *editorWidget = editor->editorWidget();
    const auto closeEditor = qScopeGuard([editor] {
        Core::Find::hideFindToolBar();
        Core::EditorManager::closeEditors({editor}, false);
    });
    Core::Command *command = Core::ActionManager::command(Core::Constants::FIND_IN_DOCUMENT);
    QVERIFY(command);

    // Find/Replace takes the selection of the lines 3 and 4 as the text to search for, not as
    // where to search
    selectLinesThreeAndFour(editorWidget);
    QVERIFY(focusForFind(editorWidget));
    QVERIFY(command->action()->isEnabled());
    command->action()->trigger();

    auto find = Utils::Aggregation::query<Core::IFindSupport>(editorWidget);
    QVERIFY(find);
    QCOMPARE(find->replaceAll("a\\nb", "X", Utils::FindRegularExpression), 3);
    QCOMPARE(editorWidget->textDocument()->plainText(), QString("X\nX\nX"));
}

void FindReplaceTest::testFindInSelectionCommand()
{
    if (!activateMainWindow())
        QSKIP("The main window does not become active.");
    BaseTextEditor *editor = openTextEditor("a\nb\na\nb\na\nb", "find.txt");
    QVERIFY(editor);
    TextEditorWidget *editorWidget = editor->editorWidget();
    const auto closeEditor = qScopeGuard([editor] {
        Core::Find::hideFindToolBar();
        Core::EditorManager::closeEditors({editor}, false);
    });
    Core::Command *command = Core::ActionManager::command(Core::Constants::FIND_IN_SELECTION);
    QVERIFY(command);

    // Only the lines 3 and 4 are in the scope of the search
    selectLinesThreeAndFour(editorWidget);
    QVERIFY(focusForFind(editorWidget));
    QVERIFY(command->action()->isEnabled());
    command->action()->trigger();

    auto find = Utils::Aggregation::query<Core::IFindSupport>(editorWidget);
    QVERIFY(find);
    QCOMPARE(find->replaceAll("a\\nb", "X", Utils::FindRegularExpression), 1);
    QCOMPARE(editorWidget->textDocument()->plainText(), QString("a\nb\nX\na\nb"));
}

void FindReplaceTest::testCurrentFindString()
{
    QStringList lines;
    for (int i = 1; i <= 25; ++i)
        lines << QString("line %1").arg(i);
    BaseTextEditor *editor = openTextEditor(lines.join('\n'), "find.txt");
    QVERIFY(editor);
    TextEditorWidget *editorWidget = editor->editorWidget();
    Core::BaseTextFind find(editorWidget);

    const auto selectLines = [&](int first, int last) {
        const QTextDocument *document = editorWidget->document();
        QTextCursor cursor(document->findBlockByNumber(first - 1));
        cursor.setPosition(document->findBlockByNumber(last - 1).position()
                               + document->findBlockByNumber(last - 1).length() - 1,
                           QTextCursor::KeepAnchor);
        editorWidget->setTextCursor(cursor);
    };

    // The word under the cursor, as before
    QTextCursor cursor(editorWidget->document());
    cursor.setPosition(2);
    editorWidget->setTextCursor(cursor);
    QCOMPARE(find.currentFindString(), QString("line"));

    // A selection on one line, as before
    cursor.setPosition(0);
    cursor.setPosition(6, QTextCursor::KeepAnchor);
    editorWidget->setTextCursor(cursor);
    QCOMPARE(find.currentFindString(), QString("line 1"));

    // A selection over several lines, with the line breaks of the document
    selectLines(2, 4);
    QCOMPARE(find.currentFindString(), QString("line 2\nline 3\nline 4"));

    // Up to twenty lines
    selectLines(1, 20);
    QCOMPARE(find.currentFindString(), lines.mid(0, 20).join('\n'));

    // More is not taken as the text to search for
    selectLines(1, 21);
    QVERIFY(find.currentFindString().isEmpty());

    // Whole lines, selected up to the start of the line after them, as with Shift+Down
    const auto selectUpToTheStartOfLine = [&](int first, int last) {
        const QTextDocument *document = editorWidget->document();
        QTextCursor lines(document->findBlockByNumber(first - 1));
        lines.setPosition(document->findBlockByNumber(last - 1).position(),
                          QTextCursor::KeepAnchor);
        editorWidget->setTextCursor(lines);
    };
    selectUpToTheStartOfLine(1, 21);
    QCOMPARE(find.currentFindString(), lines.mid(0, 20).join('\n') + '\n');
    selectUpToTheStartOfLine(1, 22);
    QVERIFY(find.currentFindString().isEmpty());

    Core::EditorManager::closeEditors({editor}, false);
}

void FindReplaceTest::testReplaceAllAcrossLinesAdjacent()
{
    BaseTextEditor *editor = openTextEditor("ab\nab\nab\ncd", "find.txt");
    QVERIFY(editor);
    TextEditorWidget *editorWidget = editor->editorWidget();

    // The replacement is longer than the match and the matches follow each other
    Core::BaseTextFind<TextEditorWidget> find(editorWidget);
    QCOMPARE(find.replaceAll("b\\na", "[\\n]", Utils::FindRegularExpression), 2);
    QCOMPARE(editorWidget->textDocument()->plainText(), QString("a[\n][\n]b\ncd"));

    Core::EditorManager::closeEditors({editor}, false);
}

QObject *createFindReplaceTest()
{
    return new FindReplaceTest;
}

class SnippetTest final : public QObject
{
    Q_OBJECT

private slots:
    void testVariableMirroring();
};

void SnippetTest::testVariableMirroring()
{
    BaseTextEditor *editor = openTextEditor(QString(), "snippet.cpp");
    QVERIFY(editor);
    TextEditorWidget *editorWidget = editor->editorWidget();

    // A class template snippet echoing the "class derived from QObject" snippet:
    // the "name" variable occurs three times and must stay in sync, and a
    // second "base" variable is reached by tabbing to the next placeholder.
    const QString snippet = "class $name$ : public $base$ {\n"
                            "    Q_OBJECT\n"
                            "public:\n"
                            "    $name$() {}\n"
                            "    virtual ~$name$() {}\n"
                            "};";
    editorWidget->insertCodeSnippet(0, snippet, &Snippet::parse);

    // The first variable is active; typing a value renames every occurrence.
    QTest::keyClicks(editorWidget, "MyName");

    // Tab jumps to the next placeholder ("base"); fill it too.
    QTest::keyClick(editorWidget, Qt::Key_Tab);
    QTest::keyClicks(editorWidget, "QObject");

    const QString text = editorWidget->textDocument()->plainText();
    QCOMPARE(text.count("MyName"), 3);
    QVERIFY(text.contains("public QObject"));
    QVERIFY(!text.contains("name")); // the "name" placeholder was filled everywhere
    QVERIFY(!text.contains("base")); // the "base" placeholder was reached via Tab

    Core::EditorManager::closeEditors({editor}, false);
}

QObject *createSnippetTest()
{
    return new SnippetTest;
}

class PrintTest final : public QObject
{
    Q_OBJECT

private slots:
    void testPrintsSelectedLinesOnly()
    {
        const QString all = "line1\nline2\nline3\nline4\n";
        QString title = "print.txt";
        Core::IEditor *editor = Core::EditorManager::openEditorWithContents(
            Core::Constants::K_DEFAULT_TEXT_EDITOR_ID, &title, all.toUtf8());
        QVERIFY(editor);
        const QScopeGuard cleanup([&] { Core::EditorManager::closeEditors({editor}, false); });
        auto baseEditor = qobject_cast<BaseTextEditor *>(editor);
        QVERIFY(baseEditor);
        TextEditorWidget *editorWidget = baseEditor->editorWidget();
        QVERIFY(editorWidget);

        QCOMPARE(editorWidget->textToPrint(false), all);

        // A selection reaching into a line prints that line as a whole.
        QTextCursor cursor = editorWidget->textCursor();
        cursor.setPosition(all.indexOf("ne2"));
        cursor.setPosition(all.indexOf("ne3"), QTextCursor::KeepAnchor);
        editorWidget->setTextCursor(cursor);
        QCOMPARE(editorWidget->textToPrint(true), QString("line2\nline3"));

        // A selection ending where a line starts does not print that line.
        cursor.setPosition(all.indexOf("line2"));
        cursor.setPosition(all.indexOf("line3"), QTextCursor::KeepAnchor);
        editorWidget->setTextCursor(cursor);
        QCOMPARE(editorWidget->textToPrint(true), QString("line2"));

        // Without a selection the whole document is printed.
        cursor.clearSelection();
        editorWidget->setTextCursor(cursor);
        QCOMPARE(editorWidget->textToPrint(false), all);
    }
};

QObject *createPrintTest()
{
    return new PrintTest;
}

class FollowSymbolTest final : public QObject
{
    Q_OBJECT

private slots:
    void testAnswerWithoutLookup()
    {
        QString title = "follow_symbol.txt";
        Core::IEditor *editor = Core::EditorManager::openEditorWithContents(
            Core::Constants::K_DEFAULT_TEXT_EDITOR_ID, &title, "nothing to follow\n");
        QVERIFY(editor);
        const QScopeGuard cleanup([&] { Core::EditorManager::closeEditors({editor}, false); });
        auto baseEditor = qobject_cast<BaseTextEditor *>(editor);
        QVERIFY(baseEditor);
        TextEditorWidget *editorWidget = baseEditor->editorWidget();
        QVERIFY(editorWidget);

        std::optional<bool> opened;
        editorWidget->openLinkUnderCursor([&opened](bool wasOpened) { opened = wasOpened; });

        QTRY_VERIFY(opened.has_value());
        QVERIFY(!*opened);
    }
};

QObject *createFollowSymbolTest()
{
    return new FollowSymbolTest;
}

class ScopeEndAnnotationsTest final : public QObject
{
    Q_OBJECT

private slots:
    void testLongScopeIsLabeled();
    void testLabels_data();
    void testLabels();
    void testFollowsEdits();
    void testDisabled();
    void testIfdefedOutBraces();
    void testBraceRuns_data();
    void testBraceRuns();
    void testToolTipOnHover();
    void testToolTipOfBraceRun();
    void testHoverHandlerGoesFirst();
    void testToolTipReceiverGoesFirst();
    void testAnnotationOnHover();
    void testAnnotationOnHoverAfterEnter();
    void testAnnotationOnHoverAfterScroll();
    void testAnnotationKeepsItsPlace_data();
    void testAnnotationKeepsItsPlace();
    void testLongLabelLeavesRoomForAnnotation();
    void testEditRepaintsOnlyWhatShowsLabels();

private:
    static void storeParentheses(QTextDocument *document);
    static QString body(int lines);
    // The labels of all lines, by 1-based line number.
    static QMap<int, QString> labels(const TextEditorWidget &widget);
    static DisplaySettingsData scopeEndSettings(int minimumLines = 20);
    static std::unique_ptr<TextEditorWidget> createWidget(const QString &text,
                                                          const DisplaySettingsData &settings);
};

// The editors get these from their highlighters.
void ScopeEndAnnotationsTest::storeParentheses(QTextDocument *document)
{
    for (QTextBlock block = document->begin(); block.isValid(); block = block.next()) {
        Parentheses parentheses;
        const QString text = block.text();
        for (int i = 0; i < text.size(); ++i) {
            if (text.at(i) == '{')
                parentheses.append(Parenthesis(Parenthesis::Opened, '{', i));
            else if (text.at(i) == '}')
                parentheses.append(Parenthesis(Parenthesis::Closed, '}', i));
        }
        TextBlockUserData::setParentheses(block, parentheses);
    }
}

QString ScopeEndAnnotationsTest::body(int lines)
{
    QString result;
    for (int i = 0; i < lines; ++i)
        result += QString("    statement%1;\n").arg(i);
    return result;
}

QMap<int, QString> ScopeEndAnnotationsTest::labels(const TextEditorWidget &widget)
{
    QMap<int, QString> result;
    for (QTextBlock block = widget.document()->begin(); block.isValid(); block = block.next()) {
        const QString label = widget.scopeEndLabel(block);
        if (!label.isEmpty())
            result.insert(block.blockNumber() + 1, label);
    }
    return result;
}

DisplaySettingsData ScopeEndAnnotationsTest::scopeEndSettings(int minimumLines)
{
    DisplaySettingsData settings = displaySettings().data();
    settings.m_displayScopeEnds = true;
    settings.m_scopeEndDisplay = ScopeEndDisplay::Annotation;
    settings.m_scopeEndMinimumLines = minimumLines;
    return settings;
}

std::unique_ptr<TextEditorWidget> ScopeEndAnnotationsTest::createWidget(
    const QString &text, const DisplaySettingsData &settings)
{
    auto document = QSharedPointer<TextDocument>::create();
    document->setPlainText(text);
    storeParentheses(document->document());
    auto widget = std::make_unique<TextEditorWidget>();
    widget->setTextDocument(document);
    widget->setDisplaySettings(settings);
    return widget;
}

void ScopeEndAnnotationsTest::testLongScopeIsLabeled()
{
    const auto widget = createWidget("void f()\n{\n" + body(25) + "}\n"
                                     "void g()\n{\n" + body(3) + "}\n",
                                     scopeEndSettings());

    // Only the long one, and on its closing line.
    const QMap<int, QString> found = labels(*widget);
    QCOMPARE(found.size(), 1);
    QCOMPARE(found.value(2 + 25 + 1), QString("void f()"));
}

void ScopeEndAnnotationsTest::testLabels_data()
{
    QTest::addColumn<QString>("header");
    QTest::addColumn<QString>("close");
    QTest::addColumn<QString>("label"); // empty: no label

    QTest::newRow("brace on own line") << "int f(int a)\n{" << "}" << "int f(int a)";
    QTest::newRow("brace after condition") << "if (debug) {" << "}" << "if (debug)";
    QTest::newRow("else if") << "} else if (x) {" << "}" << "else if (x)";
    QTest::newRow("else if over several lines")
        << "} else if (first &&\n           second) {" << "}" << "else if (first && second)";
    QTest::newRow("else if, brace on own line") << "} else if (x)\n{" << "}" << "else if (x)";
    QTest::newRow("indented") << "        while (!file.atEnd()) {" << "        }"
                              << "while (!file.atEnd())";
    QTest::newRow("parameters over several lines")
        << "void Foo::bar(int a,\n          int b) const\n{" << "}"
        << "void Foo::bar(int a, int b) const";
    QTest::newRow("end of statement") << "auto l = [] {" << "};" << "auto l = []";
    QTest::newRow("call with a trailing lambda")
        << "connect(object, &Object::changed,\n        this, [this] {" << "});"
        << "connect(object, &Object::changed, this, [this]";
    QTest::newRow("call with a trailing lambda over three lines")
        << "connect(object,\n        &Object::changed,\n        this, [this] {" << "});"
        << "connect(object, &Object::changed, this, [this]";
    QTest::newRow("sibling in a list") << "Item {\n},\nTransition {" << "}" << "Transition";
    QTest::newRow("sibling in a recipe") << "parallel,\nonGroupSetup(onSetup),\nGroup {" << "}"
                                         << "Group";
    QTest::newRow("sibling of a call with a trailing lambda")
        << "parallel,\nonGroupDone([this] {" << "})" << "onGroupDone([this]";
    QTest::newRow("trailing lambda in a recipe")
        << "const Group recipe {\n    onGroupSetup(onSetup),\n    ProcessTask(onSetup, onDone),\n"
           "    onGroupDone([this] {"
        << "    })" << "onGroupDone([this]";
    QTest::newRow("lambda captures over several lines")
        << "const auto handler = [this, document,\n                      line](int result) {"
        << "};" << "const auto handler = [this, document, line](int result)";
    QTest::newRow("lifetimes") << "#[inline]\nfn parse<'a>(input: &'a str) {" << "}"
                               << "fn parse<'a>(input: &'a str)";
    QTest::newRow("apostrophes in comments")
        << "// Can't fail.\n/* isn't virtual */ int f(int a /* can't be 0 */)\n{" << "}"
        << "/* isn't virtual */ int f(int a /* can't be 0 */)";
    QTest::newRow("sibling with a comparison")
        << "parallel,\nonGroupSetup([this] { return count < 0; }),\nonGroupDone([this] {" << "})"
        << "onGroupDone([this]";
    QTest::newRow("condition over several lines")
        << "if first &&\n    second {" << "}" << "if first && second";
    QTest::newRow("condition continued by an operator")
        << "if first\n    || second {" << "}" << "if first || second";
    QTest::newRow("element after a sibling")
        << "[\n  {\n    \"a\": 1\n  },\n  {" << "  }," << "";
    QTest::newRow("element after a call with a trailing lambda")
        << "call(a, [] {\n}),\n{" << "}," << "";
    QTest::newRow("first element") << "[\n  {" << "  }," << "";
    QTest::newRow("code after brace") << "if (a) {" << "} else {" << "";
    QTest::newRow("code before brace") << "if (a) {" << "    x; }" << "";
}

void ScopeEndAnnotationsTest::testLabels()
{
    QFETCH(QString, header);
    QFETCH(QString, close);
    QFETCH(QString, label);

    const auto widget = createWidget(header + "\n" + body(25) + close + "\n", scopeEndSettings());

    const QMap<int, QString> found = labels(*widget);
    if (label.isEmpty()) {
        QVERIFY(found.isEmpty());
    } else {
        QCOMPARE(found.size(), 1);
        QCOMPARE(found.first(), label);
    }
}

void ScopeEndAnnotationsTest::testFollowsEdits()
{
    const auto widget = createWidget("if (a) {\n" + body(25) + "}\n", scopeEndSettings());
    QTextDocument *document = widget->document();
    QCOMPARE(labels(*widget), (QMap<int, QString>{{27, "if (a)"}}));

    // Lines inserted above move the label with its line.
    QTextCursor cursor(document);
    cursor.insertText("// comment\n// comment\n");
    storeParentheses(document);
    QCOMPARE(labels(*widget), (QMap<int, QString>{{29, "if (a)"}}));

    // Changing the header changes the label...
    cursor.setPosition(document->findBlockByNumber(2).position());
    cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
    cursor.insertText("while (b) {");
    storeParentheses(document);
    QCOMPARE(labels(*widget), (QMap<int, QString>{{29, "while (b)"}}));

    // ... and shortening the scope removes it.
    cursor.setPosition(document->findBlockByNumber(3).position());
    cursor.setPosition(document->findBlockByNumber(13).position(), QTextCursor::KeepAnchor);
    cursor.removeSelectedText();
    storeParentheses(document);
    QVERIFY(labels(*widget).isEmpty());
}

void ScopeEndAnnotationsTest::testDisabled()
{
    DisplaySettingsData settings = scopeEndSettings();
    settings.m_displayScopeEnds = false;
    const auto widget = createWidget("if (a) {\n" + body(25) + "}\n", settings);
    QVERIFY(labels(*widget).isEmpty());

    settings.m_displayScopeEnds = true;
    widget->setDisplaySettings(settings);
    QCOMPARE(labels(*widget).size(), 1);

    settings.m_displayScopeEnds = false;
    widget->setDisplaySettings(settings);
    QVERIFY(labels(*widget).isEmpty());
}

void ScopeEndAnnotationsTest::testIfdefedOutBraces()
{
    // Both branches open a scope, and only one of them is closed.
    const QString text = "void f()\n"                    // 1
                         "{\n"
                         "#ifdef Q_OS_WIN\n"
                         "    if (useWinApi()) {\n"      // 4
                         "#else\n"
                         "    if (usePosix()) {\n"       // 6
                         "#endif\n"
                         + body(25)
                         + "    }\n"                     // 33
                         + body(25)
                         + "}\n";                        // 59
    const auto widget = createWidget(text, scopeEndSettings());
    TextBlockUserData::setIfdefedOut(widget->document()->findBlockByNumber(3));

    QCOMPARE(labels(*widget),
             (QMap<int, QString>{{33, "if (usePosix())"}, {59, "void f()"}}));
}

void ScopeEndAnnotationsTest::testBraceRuns_data()
{
    QTest::addColumn<QString>("text"); // the last line closes several scopes
    QTest::addColumn<QStringList>("braceLabels"); // of the braces in the last line, in order

    const QString function = "void f()\n{\n" + body(25);
    QTest::newRow("three scopes") << function + "    for (x) {\n" + body(25) + "        if (a) {\n"
                                         + body(25) + "}}}"
                                  << QStringList{"if (a)", "for (x)", "void f()"};
    QTest::newRow("short inner scope")
        << function + "    for (x) {\n" + body(25) + "        if (a) {\n" + body(3) + "}}}"
        << QStringList{"", "for (x)", "void f()"};
    QTest::newRow("short scopes") << "void f()\n{\n    if (a) {\n" + body(3) + "    } }"
                                  << QStringList{"", ""};
    QTest::newRow("indented with spaces")
        << function + "    if (a) {\n" + body(25) + "    }  }  "
        << QStringList{"if (a)", "void f()"};
    QTest::newRow("trailing lambda in a call")
        << "call(a, [] {\n    if (b) {\n" + body(25) + "    }});"
        << QStringList{"if (b)", "call(a, []"};
    QTest::newRow("end of statement") << "auto l = [] {\n    if (b) {\n" + body(25) + "}  } ;"
                                      << QStringList{"if (b)", "auto l = []"};
}

void ScopeEndAnnotationsTest::testBraceRuns()
{
    QFETCH(QString, text);
    QFETCH(QStringList, braceLabels);

    const auto widget = createWidget(text, scopeEndSettings());
    const QTextBlock block = widget->document()->lastBlock();
    const QString line = block.text();

    // The line is labeled with the outermost scope.
    QCOMPARE(widget->scopeEndLabel(block), braceLabels.last());
    QCOMPARE(labels(*widget).size(), braceLabels.last().isEmpty() ? 0 : 1);

    // Each brace has the label of its own scope, and all the other characters of the line,
    // and the end of it, that of the outermost scope.
    qsizetype brace = 0;
    for (int i = 0; i <= line.size(); ++i) {
        const bool isBrace = i < line.size() && line.at(i) == '}';
        const QString expected = isBrace ? braceLabels.value(brace++) : braceLabels.last();
        QVERIFY2(widget->scopeEndLabel(block, i) == expected,
                 qPrintable(QString("Position %1: \"%2\" instead of \"%3\"")
                                .arg(i)
                                .arg(widget->scopeEndLabel(block, i), expected)));
    }
    QCOMPARE(brace, braceLabels.size());
}

// The visible text of the tool tip, which is a label or a group of them.
static QStringList toolTipTexts()
{
    QStringList result;
    for (QWidget *topLevel : QApplication::topLevelWidgets()) {
        if (!topLevel->isVisible())
            continue;
        QList<QLabel *> labels = topLevel->findChildren<QLabel *>();
        if (auto label = qobject_cast<QLabel *>(topLevel))
            labels.prepend(label);
        for (QLabel *label : std::as_const(labels)) {
            if (label->isVisible() && !label->text().isEmpty())
                result.append(label->text());
        }
    }
    return result;
}

void ScopeEndAnnotationsTest::testToolTipOnHover()
{
    QString title = "scope.cpp";
    Core::IEditor *editor = Core::EditorManager::openEditorWithContents(
        Core::Constants::K_DEFAULT_TEXT_EDITOR_ID, &title,
        "if (a) {\n" + body(25).toUtf8() + "}\n");
    QVERIFY(editor);
    const QScopeGuard cleanup([&] {
        Utils::ToolTip::hideImmediately();
        Core::EditorManager::closeEditors({editor}, false);
    });
    auto baseEditor = qobject_cast<BaseTextEditor *>(editor);
    QVERIFY(baseEditor);
    TextEditorWidget *widget = baseEditor->editorWidget();
    widget->resize(600, 600);
    widget->show();

    TextDocument *document = baseEditor->textDocument();
    storeParentheses(document->document());
    DisplaySettingsData settings = scopeEndSettings();
    settings.m_scopeEndDisplay = ScopeEndDisplay::ToolTip;
    widget->setDisplaySettings(settings);

    const auto hover = [&](int blockNumber) {
        QTextCursor cursor(document->document()->findBlockByNumber(blockNumber));
        const QPoint position = widget->cursorRect(cursor).center();
        QHelpEvent event(QEvent::ToolTip, position, widget->viewport()->mapToGlobal(position));
        Utils::ToolTip::hideImmediately();
        QCoreApplication::sendEvent(widget->viewport(), &event);
        return Utils::ToolTip::isVisible();
    };
    QVERIFY(hover(26)); // the closing brace
    QVERIFY(!hover(5)); // a line without a scope end

    // The label is also there for the lines that are too crowded to hold it.
    settings.m_scopeEndDisplay = ScopeEndDisplay::Annotation;
    widget->setDisplaySettings(settings);
    QVERIFY(hover(26));
    QVERIFY(!hover(5));
    settings.m_scopeEndDisplay = ScopeEndDisplay::OnHover;
    widget->setDisplaySettings(settings);
    QVERIFY(hover(26));
}

void ScopeEndAnnotationsTest::testToolTipOfBraceRun()
{
    QString title = "scope.cpp";
    Core::IEditor *editor = Core::EditorManager::openEditorWithContents(
        Core::Constants::K_DEFAULT_TEXT_EDITOR_ID, &title,
        "void f()\n{\n    if (a) {\n" + body(25).toUtf8() + "    }};\n");
    QVERIFY(editor);
    const QScopeGuard cleanup([&] {
        Utils::ToolTip::hideImmediately();
        Core::EditorManager::closeEditors({editor}, false);
    });
    auto baseEditor = qobject_cast<BaseTextEditor *>(editor);
    QVERIFY(baseEditor);
    TextEditorWidget *widget = baseEditor->editorWidget();
    widget->resize(600, 600);
    widget->show();

    TextDocument *document = baseEditor->textDocument();
    storeParentheses(document->document());
    DisplaySettingsData settings = scopeEndSettings();
    settings.m_scopeEndDisplay = ScopeEndDisplay::ToolTip;
    widget->setDisplaySettings(settings);

    // Hovers the character at positionInBlock in the closing line, at three quarters of its
    // width, which is closer to the cursor position after it than to the one in front of it.
    const QTextBlock closingLine = document->document()->findBlockByNumber(28);
    QCOMPARE(closingLine.text(), QString("    }};"));
    const auto hover = [&](int positionInBlock) {
        QTextCursor cursor(closingLine);
        cursor.setPosition(closingLine.position() + positionInBlock);
        const QRect before = widget->cursorRect(cursor);
        cursor.movePosition(QTextCursor::NextCharacter);
        const QRect after = widget->cursorRect(cursor);
        const QPoint position((before.center().x() + 3 * after.center().x()) / 4,
                              before.center().y());
        QHelpEvent event(QEvent::ToolTip, position, widget->viewport()->mapToGlobal(position));
        Utils::ToolTip::hideImmediately();
        QCoreApplication::sendEvent(widget->viewport(), &event);
        const QStringList texts = toolTipTexts();
        const bool inner = texts.contains("if (a)");
        const bool outer = texts.contains("void f()");
        // Exactly one of the labels, or what is shown instead.
        if (inner == outer)
            return texts.join(", ");
        return inner ? QString("if (a)") : QString("void f()");
    };
    QCOMPARE(hover(4), QString("if (a)"));
    QCOMPARE(hover(5), QString("void f()"));
    QCOMPARE(hover(6), QString("void f()")); // the ';'
    QCOMPARE(hover(1), QString("void f()")); // the indentation
}

void ScopeEndAnnotationsTest::testHoverHandlerGoesFirst()
{
    QString title = "scope.cpp";
    Core::IEditor *editor = Core::EditorManager::openEditorWithContents(
        Core::Constants::K_DEFAULT_TEXT_EDITOR_ID, &title,
        "if (a) {\n" + body(25).toUtf8() + "}\n");
    QVERIFY(editor);
    auto baseEditor = qobject_cast<BaseTextEditor *>(editor);
    QVERIFY(baseEditor);
    TextEditorWidget *widget = baseEditor->editorWidget();
    // Shows the tool tips of extra selections, which is where diagnostics put theirs.
    BaseHoverHandler handler;
    widget->addHoverHandler(&handler);
    const QScopeGuard cleanup([&] {
        Utils::ToolTip::hideImmediately();
        widget->removeHoverHandler(&handler);
        Core::EditorManager::closeEditors({editor}, false);
    });
    widget->resize(600, 600);
    widget->show();

    TextDocument *document = baseEditor->textDocument();
    storeParentheses(document->document());
    DisplaySettingsData settings = scopeEndSettings();
    settings.m_scopeEndDisplay = ScopeEndDisplay::ToolTip;
    widget->setDisplaySettings(settings);

    const QTextBlock closingLine = document->document()->findBlockByNumber(26);
    const auto hoverClosingLine = [&] {
        const QPoint position = widget->cursorRect(QTextCursor(closingLine)).center();
        QHelpEvent event(QEvent::ToolTip, position, widget->viewport()->mapToGlobal(position));
        Utils::ToolTip::hideImmediately();
        QCoreApplication::sendEvent(widget->viewport(), &event);
    };

    // With nothing else to show, the line shows the label.
    hoverClosingLine();
    QTRY_VERIFY(toolTipTexts().contains("if (a)"));

    const QString diagnostic = "non-void function does not return a value";
    QTextEdit::ExtraSelection selection;
    selection.cursor = QTextCursor(closingLine);
    selection.cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
    selection.format.setToolTip(diagnostic);
    widget->setExtraSelections(TextEditorWidget::CodeWarningsSelection, {selection});

    hoverClosingLine();
    QTRY_VERIFY(toolTipTexts().contains(diagnostic));
    QVERIFY2(!toolTipTexts().contains("if (a)"), qPrintable(toolTipTexts().join(", ")));
}

void ScopeEndAnnotationsTest::testToolTipReceiverGoesFirst()
{
    QString title = "scope.cpp";
    Core::IEditor *editor = Core::EditorManager::openEditorWithContents(
        Core::Constants::K_DEFAULT_TEXT_EDITOR_ID, &title,
        "if (a) {\n" + body(25).toUtf8() + "}\n");
    QVERIFY(editor);
    const QScopeGuard cleanup([&] {
        Utils::ToolTip::hideImmediately();
        Core::EditorManager::closeEditors({editor}, false);
    });
    auto baseEditor = qobject_cast<BaseTextEditor *>(editor);
    QVERIFY(baseEditor);
    TextEditorWidget *widget = baseEditor->editorWidget();
    widget->resize(600, 600);
    widget->show();

    TextDocument *document = baseEditor->textDocument();
    storeParentheses(document->document());
    DisplaySettingsData settings = scopeEndSettings();
    settings.m_scopeEndDisplay = ScopeEndDisplay::ToolTip;
    widget->setDisplaySettings(settings);

    // Like the GLSL editor, which shows its diagnostics this way.
    const QString diagnostic = "syntax error, unexpected '}'";
    bool hasDiagnostic = false;
    QObject receiver;
    connect(widget, &TextEditorWidget::tooltipRequested, &receiver,
            [&](const QPoint &point, int) {
                if (hasDiagnostic)
                    Utils::ToolTip::show(point, diagnostic, widget);
                else
                    Utils::ToolTip::hide();
            });

    const auto hoverClosingLine = [&] {
        const QTextBlock closingLine = document->document()->findBlockByNumber(26);
        const QPoint position = widget->cursorRect(QTextCursor(closingLine)).center();
        QHelpEvent event(QEvent::ToolTip, position, widget->viewport()->mapToGlobal(position));
        Utils::ToolTip::hideImmediately();
        QCoreApplication::sendEvent(widget->viewport(), &event);
    };

    // With nothing else to show, the line shows the label.
    hoverClosingLine();
    QTRY_VERIFY(toolTipTexts().contains("if (a)"));

    hasDiagnostic = true;
    hoverClosingLine();
    QTRY_VERIFY(toolTipTexts().contains(diagnostic));
    QVERIFY2(!toolTipTexts().contains("if (a)"), qPrintable(toolTipTexts().join(", ")));
}

void ScopeEndAnnotationsTest::testAnnotationOnHover()
{
    QString title = "scope.cpp";
    Core::IEditor *editor = Core::EditorManager::openEditorWithContents(
        Core::Constants::K_DEFAULT_TEXT_EDITOR_ID, &title,
        "if (a) {\n" + body(25).toUtf8() + "}\n");
    QVERIFY(editor);
    const QScopeGuard cleanup([&] { Core::EditorManager::closeEditors({editor}, false); });
    auto baseEditor = qobject_cast<BaseTextEditor *>(editor);
    QVERIFY(baseEditor);
    TextEditorWidget *widget = baseEditor->editorWidget();
    widget->resize(600, 600);
    widget->show();

    TextDocument *document = baseEditor->textDocument();
    storeParentheses(document->document());
    DisplaySettingsData settings = scopeEndSettings();
    settings.m_scopeEndDisplay = ScopeEndDisplay::OnHover;
    widget->setDisplaySettings(settings);

    const auto rowAt = [&](int blockNumber) {
        QTextCursor cursor(document->document()->findBlockByNumber(blockNumber));
        const QRect cursorRect = widget->cursorRect(cursor);
        return QRect(0, cursorRect.top(), widget->viewport()->width(), cursorRect.height());
    };
    const auto moveOnto = [&](int blockNumber) {
        const QPoint position = rowAt(blockNumber).center();
        QMouseEvent move(QEvent::MouseMove, position, widget->viewport()->mapToGlobal(position),
                         Qt::NoButton, Qt::NoButton, Qt::NoModifier);
        QCoreApplication::sendEvent(widget->viewport(), &move);
    };
    const auto closingLine = [&] { return widget->viewport()->grab(rowAt(26)).toImage(); };

    const QImage idle = closingLine();
    moveOnto(26);
    QVERIFY(closingLine() != idle);
    moveOnto(5);
    QCOMPARE(closingLine(), idle);

    moveOnto(26);
    QVERIFY(closingLine() != idle);
    QEvent leave(QEvent::Leave);
    QCoreApplication::sendEvent(widget, &leave);
    QCOMPARE(closingLine(), idle);

    // Moving onto the line numbers or the scroll bar leaves only the viewport.
    moveOnto(26);
    QVERIFY(closingLine() != idle);
    QEvent leaveViewport(QEvent::Leave);
    QCoreApplication::sendEvent(widget->viewport(), &leaveViewport);
    QCOMPARE(closingLine(), idle);

    // Always shown, the line looks the same whether the mouse is on it or not.
    settings.m_scopeEndDisplay = ScopeEndDisplay::Annotation;
    widget->setDisplaySettings(settings);
    const QImage shown = closingLine();
    QVERIFY(shown != idle);
    moveOnto(5);
    QCOMPARE(closingLine(), shown);

    // The switch for the other line annotations has no say in it.
    settings.m_displayAnnotations = false;
    widget->setDisplaySettings(settings);
    QCOMPARE(closingLine(), shown);
    settings.m_displayAnnotations = true;

    // The group box of the settings page turns all of it off, whatever the mode.
    settings.m_displayScopeEnds = false;
    widget->setDisplaySettings(settings);
    QCOMPARE(closingLine(), idle);
}

void ScopeEndAnnotationsTest::testAnnotationOnHoverAfterEnter()
{
    QString title = "scope.cpp";
    Core::IEditor *editor = Core::EditorManager::openEditorWithContents(
        Core::Constants::K_DEFAULT_TEXT_EDITOR_ID, &title,
        "if (a) {\n" + body(25).toUtf8() + "}\n");
    QVERIFY(editor);
    const QScopeGuard cleanup([&] { Core::EditorManager::closeEditors({editor}, false); });
    auto baseEditor = qobject_cast<BaseTextEditor *>(editor);
    QVERIFY(baseEditor);
    TextEditorWidget *widget = baseEditor->editorWidget();
    widget->resize(600, 600);
    widget->show();

    TextDocument *document = baseEditor->textDocument();
    storeParentheses(document->document());
    DisplaySettingsData settings = scopeEndSettings();
    settings.m_scopeEndDisplay = ScopeEndDisplay::OnHover;
    widget->setDisplaySettings(settings);

    const auto rowAt = [&](int blockNumber) {
        QTextCursor cursor(document->document()->findBlockByNumber(blockNumber));
        const QRect cursorRect = widget->cursorRect(cursor);
        return QRect(0, cursorRect.top(), widget->viewport()->width(), cursorRect.height());
    };
    const auto rowImage = [&](int blockNumber) {
        return widget->viewport()->grab(rowAt(blockNumber)).toImage();
    };

    const QImage idle = rowImage(26);
    const QPoint position = rowAt(26).center();
    QMouseEvent move(QEvent::MouseMove, position, widget->viewport()->mapToGlobal(position),
                     Qt::NoButton, Qt::NoButton, Qt::NoModifier);
    QCoreApplication::sendEvent(widget->viewport(), &move);
    QVERIFY(rowImage(26) != idle);

    // A line inserted above moves the closing brace down, and the mouse stays where it is.
    QTextCursor cursor(document->document());
    cursor.insertText("\n");
    storeParentheses(document->document());
    QCoreApplication::processEvents();
    QCOMPARE(widget->cursorForPosition(position).blockNumber(), 26);

    // The mouse is on a line without a scope end now, and the closing brace has no label.
    // Leaving the editor shows how the two rows look without any hovering.
    const QImage underMouse = rowImage(26);
    const QImage closingBrace = rowImage(27);
    QEvent leave(QEvent::Leave);
    QCoreApplication::sendEvent(widget, &leave);
    QCOMPARE(rowImage(26), underMouse);
    QCOMPARE(rowImage(27), closingBrace);
}

void ScopeEndAnnotationsTest::testAnnotationOnHoverAfterScroll()
{
    QString title = "scope.cpp";
    Core::IEditor *editor = Core::EditorManager::openEditorWithContents(
        Core::Constants::K_DEFAULT_TEXT_EDITOR_ID, &title,
        "if (a) {\n" + body(25).toUtf8() + "}\n" + body(100).toUtf8());
    QVERIFY(editor);
    const QScopeGuard cleanup([&] { Core::EditorManager::closeEditors({editor}, false); });
    auto baseEditor = qobject_cast<BaseTextEditor *>(editor);
    QVERIFY(baseEditor);
    TextEditorWidget *widget = baseEditor->editorWidget();
    widget->resize(600, 600);
    widget->show();

    TextDocument *document = baseEditor->textDocument();
    storeParentheses(document->document());
    DisplaySettingsData settings = scopeEndSettings();
    settings.m_scopeEndDisplay = ScopeEndDisplay::OnHover;
    widget->setDisplaySettings(settings);

    const auto rowAt = [&](int blockNumber) {
        QTextCursor cursor(document->document()->findBlockByNumber(blockNumber));
        const QRect cursorRect = widget->cursorRect(cursor);
        return QRect(0, cursorRect.top(), widget->viewport()->width(), cursorRect.height());
    };
    const auto closingLine = [&] { return widget->viewport()->grab(rowAt(26)).toImage(); };
    QScrollBar *scrollBar = widget->verticalScrollBar();
    scrollBar->setValue(0);

    const QImage idle = closingLine();
    const QPoint position = rowAt(26).center();
    QMouseEvent move(QEvent::MouseMove, position, widget->viewport()->mapToGlobal(position),
                     Qt::NoButton, Qt::NoButton, Qt::NoModifier);
    QCoreApplication::sendEvent(widget->viewport(), &move);
    QVERIFY(closingLine() != idle);

    // The closing brace moves away from the mouse, and takes no label with it.
    scrollBar->setValue(scrollBar->value() + 3 * rowAt(26).height());
    QCOMPARE_NE(widget->cursorForPosition(position).blockNumber(), 26);
    QCOMPARE(closingLine(), idle);
}

void ScopeEndAnnotationsTest::testAnnotationKeepsItsPlace_data()
{
    QTest::addColumn<AnnotationAlignment>("alignment");

    QTest::newRow("next to content") << AnnotationAlignment::NextToContent;
    QTest::newRow("next to margin") << AnnotationAlignment::NextToMargin;
    QTest::newRow("right side") << AnnotationAlignment::RightSide;
    QTest::newRow("between lines") << AnnotationAlignment::BetweenLines;
}

void ScopeEndAnnotationsTest::testAnnotationKeepsItsPlace()
{
    QFETCH(AnnotationAlignment, alignment);

    QString title = "scope.cpp";
    Core::IEditor *editor = Core::EditorManager::openEditorWithContents(
        Core::Constants::K_DEFAULT_TEXT_EDITOR_ID, &title,
        "if (a) {\n" + body(25).toUtf8() + "}\n");
    QVERIFY(editor);
    const QScopeGuard cleanup([&] { Core::EditorManager::closeEditors({editor}, false); });
    auto baseEditor = qobject_cast<BaseTextEditor *>(editor);
    QVERIFY(baseEditor);
    TextEditorWidget *widget = baseEditor->editorWidget();
    widget->resize(1200, 600);
    widget->show();
    widget->setVisibleWrapColumn(80);

    TextDocument *document = baseEditor->textDocument();
    storeParentheses(document->document());
    DisplaySettingsData settings = scopeEndSettings();
    settings.m_displayAnnotations = true;
    settings.m_annotationAlignment = alignment;
    settings.m_displayScopeEnds = false;
    widget->setDisplaySettings(settings);

    const auto closingLine = [&] {
        // Between lines, the first paint only makes room for the annotations below the line.
        for (int i = 0; i < 2; ++i) {
            widget->viewport()->grab();
            QCoreApplication::processEvents();
        }
        QTextCursor cursor(document->document()->findBlockByNumber(26));
        const QRect cursorRect = widget->cursorRect(cursor);
        return widget->viewport()
            ->grab(QRect(0, cursorRect.top(), widget->viewport()->width(), cursorRect.height()))
            .toImage();
    };

    const QImage idle = closingLine();
    settings.m_displayScopeEnds = true;
    widget->setDisplaySettings(settings);
    const QImage alone = closingLine();

    // Where the scope end label is drawn when it is alone on the line, in device pixels.
    QRect drawn;
    for (int x = 0; x < alone.width(); ++x) {
        for (int y = 0; y < alone.height(); ++y) {
            if (alone.pixel(x, y) != idle.pixel(x, y))
                drawn |= QRect(x, y, 1, 1);
        }
    }
    QVERIFY(!drawn.isEmpty());

    // Another annotation on the line, like the one of Git's instant blame, goes where the
    // settings put it, and leaves the scope end label where it was, even if it ranks higher.
    TextMark other(document, 27, TextMarkCategory{"Other", "TextEditor.Test.Other"});
    other.setPriority(TextMark::HighPriority);
    other.setLineAnnotation("Some Author, 2 days ago");
    const QImage both = closingLine();
    QVERIFY(both != alone);
    QCOMPARE(both.copy(drawn), alone.copy(drawn));
}

void ScopeEndAnnotationsTest::testLongLabelLeavesRoomForAnnotation()
{
    QString condition;
    for (int i = 0; condition.size() < 400; ++i)
        condition += QString(" && condition%1").arg(i);
    QString title = "scope.cpp";
    Core::IEditor *editor = Core::EditorManager::openEditorWithContents(
        Core::Constants::K_DEFAULT_TEXT_EDITOR_ID, &title,
        "if (a" + condition.toUtf8() + ") {\n" + body(25).toUtf8() + "}\n");
    QVERIFY(editor);
    const QScopeGuard cleanup([&] { Core::EditorManager::closeEditors({editor}, false); });
    auto baseEditor = qobject_cast<BaseTextEditor *>(editor);
    QVERIFY(baseEditor);
    TextEditorWidget *widget = baseEditor->editorWidget();
    widget->resize(1200, 600);
    widget->show();

    TextDocument *document = baseEditor->textDocument();
    storeParentheses(document->document());
    DisplaySettingsData settings = scopeEndSettings();
    settings.m_displayAnnotations = true;
    settings.m_annotationAlignment = AnnotationAlignment::RightSide;
    widget->setDisplaySettings(settings);

    const auto closingLine = [&] {
        widget->viewport()->grab();
        QCoreApplication::processEvents();
        QTextCursor cursor(document->document()->findBlockByNumber(26));
        const QRect cursorRect = widget->cursorRect(cursor);
        return widget->viewport()
            ->grab(QRect(0, cursorRect.top(), widget->viewport()->width(), cursorRect.height()))
            .toImage();
    };

    // The label alone fills the line.
    const QImage alone = closingLine();

    // The columns that change when the line gets an annotation, in device pixels.
    TextMark warning(document, 27, TextMarkCategory{"Warning", "TextEditor.Test.Warning"});
    warning.setLineAnnotation("non-void function does not return a value");
    const QImage both = closingLine();
    int changedLeft = both.width();
    int changedRight = -1;
    for (int x = 0; x < both.width(); ++x) {
        for (int y = 0; y < both.height(); ++y) {
            if (both.pixel(x, y) != alone.pixel(x, y)) {
                changedLeft = qMin(changedLeft, x);
                changedRight = qMax(changedRight, x);
            }
        }
    }

    // A sliver at the end of the line is not enough to show any of the annotation.
    const qreal dpr = both.devicePixelRatio();
    const int minimalContentWidth = int(widget->fontMetrics().horizontalAdvance(QString(
        settings.m_minimalAnnotationContent, 'x')) * dpr);
    QVERIFY2(changedRight - changedLeft >= minimalContentWidth,
             qPrintable(QString("changed %1 to %2, need %3")
                            .arg(changedLeft).arg(changedRight).arg(minimalContentWidth)));
}

void ScopeEndAnnotationsTest::testEditRepaintsOnlyWhatShowsLabels()
{
    QString title = "scope.cpp";
    Core::IEditor *editor = Core::EditorManager::openEditorWithContents(
        Core::Constants::K_DEFAULT_TEXT_EDITOR_ID, &title,
        "if (a) {\n" + body(25).toUtf8() + "}\n");
    QVERIFY(editor);
    const QScopeGuard cleanup([&] { Core::EditorManager::closeEditors({editor}, false); });
    auto baseEditor = qobject_cast<BaseTextEditor *>(editor);
    QVERIFY(baseEditor);
    TextEditorWidget *widget = baseEditor->editorWidget();
    widget->resize(600, 600);
    widget->show();
    QVERIFY(QTest::qWaitForWindowExposed(widget));

    TextDocument *document = baseEditor->textDocument();
    storeParentheses(document->document());

    class PaintRecorder : public QObject
    {
    public:
        bool eventFilter(QObject *, QEvent *event) override
        {
            if (event->type() == QEvent::Paint)
                painted += static_cast<QPaintEvent *>(event)->region();
            return false;
        }
        QRegion painted;
    } recorder;
    widget->viewport()->installEventFilter(&recorder);

    QTextCursor closingCursor(document->document()->findBlockByNumber(26));
    const QRect closingRow(0, widget->cursorRect(closingCursor).top(),
                           widget->viewport()->width(), 1);

    // What an edit in line 5 repaints, once the widget is done with the settings and the mouse.
    const auto paintedByEdit = [&](ScopeEndDisplay display, int hoveredBlock = -1) {
        DisplaySettingsData settings = scopeEndSettings();
        settings.m_scopeEndDisplay = display;
        widget->setDisplaySettings(settings);
        if (hoveredBlock >= 0) {
            QTextCursor hovered(document->document()->findBlockByNumber(hoveredBlock));
            const QPoint position = widget->cursorRect(hovered).center();
            QMouseEvent move(QEvent::MouseMove, position,
                             widget->viewport()->mapToGlobal(position), Qt::NoButton,
                             Qt::NoButton, Qt::NoModifier);
            QCoreApplication::sendEvent(widget->viewport(), &move);
        }
        widget->viewport()->repaint();
        recorder.painted = {};
        QTextCursor cursor(document->document()->findBlockByNumber(4));
        cursor.insertText("x");
        return QTest::qWaitFor([&] { return !recorder.painted.isEmpty(); })
                   ? recorder.painted
                   : QRegion();
    };

    // The label of the closing line may have changed.
    const QRegion annotation = paintedByEdit(ScopeEndDisplay::Annotation);
    QVERIFY(!annotation.isEmpty());
    QVERIFY(annotation.intersects(closingRow));

    // There is no label in the line to change.
    const QRegion toolTip = paintedByEdit(ScopeEndDisplay::ToolTip);
    QVERIFY(!toolTip.isEmpty());
    QVERIFY(!toolTip.intersects(closingRow));

    // Only the line under the mouse can have a label.
    const QRegion onHover = paintedByEdit(ScopeEndDisplay::OnHover, 26);
    QVERIFY(onHover.intersects(closingRow));
    const QRegion onHoverElsewhere = paintedByEdit(ScopeEndDisplay::OnHover, 9);
    QVERIFY(!onHoverElsewhere.isEmpty());
    QVERIFY(!onHoverElsewhere.intersects(closingRow));
}

QObject *createScopeEndAnnotationsTest()
{
    return new ScopeEndAnnotationsTest;
}

} // TextEditor::Internal

#include "texteditor_test.moc"

#endif // WITH_TESTS
