// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "extraencodingsettings.h"

#include "texteditortr.h"
#include "codecchooser.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>

#include <utils/layoutbuilder.h>
#include <utils/qtcsettings.h>
#include <utils/textcodec.h>

#ifdef WITH_TESTS
#include <QTest>
#endif

using namespace Utils;

namespace TextEditor {

EncodingSelectionAspect::EncodingSelectionAspect(AspectContainer *container)
    : ByteArrayAspect(container)
{}

void EncodingSelectionAspect::setAllowNone(bool allowNone)
{
    m_allowNone = allowNone;
}

void EncodingSelectionAspect::addToLayoutImpl(Layouting::Layout &parent)
{
    auto codecChooser = createSubWidget<CodecChooser>();
    if (m_allowNone)
        codecChooser->prependNone();
    codecChooser->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    const auto encoding = [this] {
        const QByteArray name = m_undoable.get();
        return name.isEmpty() ? TextEncoding() : TextEncoding(name);
    };
    m_undoable.setSilently(m_volatileValue);
    codecChooser->setAssignedEncoding(encoding());
    addLabeledItem(parent, codecChooser);

    connect(codecChooser, &CodecChooser::encodingChanged, this, [this, codecChooser] {
        m_undoable.set(undoStack(), codecChooser->currentEncoding().name());
    });
    connect(&m_undoable.m_signal, &UndoSignaller::changed, codecChooser, [codecChooser, encoding] {
        if (codecChooser->currentEncoding() != encoding())
            codecChooser->setAssignedEncoding(encoding());
    });
    // Once per change however many choosers there are.
    connect(&m_undoable.m_signal, &UndoSignaller::changed,
            this, &EncodingSelectionAspect::handleGuiChanged, Qt::UniqueConnection);
}

void EncodingSelectionAspect::volatileValueToGui()
{
    m_undoable.setWithoutUndo(m_volatileValue);
}

bool EncodingSelectionAspect::guiToVolatileValue()
{
    return updateStorage(m_volatileValue, m_undoable.get());
}

void EncodingSelectionAspect::setValue(const TextEncoding &encoding)
{
    ByteArrayAspect::setValue(encoding.name());
}

TextEncoding EncodingSelectionAspect::operator()() const
{
    const QByteArray encodingName = ByteArrayAspect::value();
    return encodingName.isEmpty() ? TextEncoding() : TextEncoding(encodingName);
}

ExtraEncodingSettings::ExtraEncodingSettings()
{
    setAutoApply(false);
    setSettingsGroup("textEditorManager");
    fallbackEncoding.setAllowNone(true);

    defaultEncoding.setSettingsKey(Core::Constants::SETTINGS_DEFAULTTEXTENCODING);
    defaultEncoding.setLabelText(Tr::tr("Default encoding:"));
    defaultEncoding.setDefaultValue(Core::EditorManager::defaultTextEncoding().name());

    fallbackEncoding.setSettingsKey("FallbackEncoding");
    fallbackEncoding.setLabelText(Tr::tr("Fallback encoding:"));
    fallbackEncoding.setToolTip(Tr::tr("If decoding with the default encoding fails, retry with this "
                                      "encoding. Select None to disable the fallback."));
    fallbackEncoding.setDefaultValue(Core::EditorManager::fallbackTextEncoding().name());

    utf8BomSetting.setSettingsKey("Utf8BomBehavior");
    utf8BomSetting.setDisplayStyle(SelectionAspect::DisplayStyle::ComboBox);
    utf8BomSetting.addOption(Tr::tr("Add If Encoding Is UTF-8"));
    utf8BomSetting.addOption(Tr::tr("Keep If Already Present"));
    utf8BomSetting.addOption(Tr::tr("Always Delete"));
    utf8BomSetting.setDefaultValue(ExtraEncodingSettingsData::OnlyKeep);
    utf8BomSetting.setLabelText(Tr::tr("UTF-8 BOM:"));
    utf8BomSetting.setToolTip(Tr::tr("<html><head/><body>\n"
        "<p>How text editors should deal with UTF-8 Byte Order Marks. The options are:</p>\n"
        "<ul ><li><i>Add If Encoding Is UTF-8:</i> always add a BOM when saving a file in UTF-8 encoding. "
        "Note that this will not work if the encoding is <i>System</i>, "
        "as the text editor does not know what it actually is.</li>\n"
        "<li><i>Keep If Already Present: </i>save the file with a BOM if it already had one when it was loaded.</li>\n"
        "<li><i>Always Delete:</i> never write an UTF-8 BOM, possibly deleting a pre-existing one.</li></ul>\n"
        "<p>Note that UTF-8 BOMs are uncommon and treated incorrectly by some editors, so it usually makes little sense to add any.</p>\n"
        "<p>This setting does <b>not</b> influence the use of UTF-16 and UTF-32 BOMs.</p></body></html>"));

    lineEndingSetting.setSettingsKey("LineEndingBehavior");
    lineEndingSetting.setDisplayStyle(SelectionAspect::DisplayStyle::ComboBox);
    lineEndingSetting.addOption(Tr::tr("Unix (LF)"));
    lineEndingSetting.addOption(Tr::tr("Windows (CRLF)"));
    lineEndingSetting.setDefaultValue(ExtraEncodingSettingsData::Unix);
    lineEndingSetting.setLabelText(Tr::tr("Default line endings:"));

    setLayouter([this] {
        using namespace Layouting;
        return Column {
            Group {
                title(Tr::tr("File Encodings")),
                Row {
                    Form {
                        defaultEncoding, br,
                        fallbackEncoding, br,
                        utf8BomSetting, br,
                        lineEndingSetting, br,
                    }, st
                }
            }
        };
    });
}

ExtraEncodingSettingsData ExtraEncodingSettings::data() const
{
    ExtraEncodingSettingsData d;
    d.m_utf8BomSetting = utf8BomSetting();
    d.m_lineEndingSetting = lineEndingSetting();
    return d;
}

void ExtraEncodingSettings::setData(const ExtraEncodingSettingsData &data)
{
    utf8BomSetting.setValue(data.m_utf8BomSetting);
    lineEndingSetting.setValue(data.m_lineEndingSetting);
}

void ExtraEncodingSettings::apply()
{
    AspectContainer::apply();
    AspectContainer::writeSettings();

    QtcSettings &s =  userSettings();

    const TextEncoding encoding(defaultEncoding());
    if (encoding.isValid())
        s.setValue(Core::Constants::SETTINGS_DEFAULTTEXTENCODING, encoding.name());

    const TextEncoding fallback(fallbackEncoding());
    if (fallback.isValid())
        s.setValue(Core::Constants::SETTINGS_FALLBACKTEXTENCODING, fallback.name());
    else
        s.remove(Core::Constants::SETTINGS_FALLBACKTEXTENCODING);

    s.setValue(Core::Constants::SETTINGS_DEFAULT_LINE_TERMINATOR,
                lineEndingSetting());
}

ExtraEncodingSettings &globalExtraEncodingSettings()
{
    static ExtraEncodingSettings theGlobalExtraEncodingSettings;
    return theGlobalExtraEncodingSettings;
}

void setupExtraEncodingSettings()
{
    globalExtraEncodingSettings().readSettings();
}

#ifdef WITH_TESTS
namespace Internal {

class EncodingSelectionAspectTest final : public QObject
{
    Q_OBJECT

private slots:
    void testUpdatesEveryChooser()
    {
        const TextEncoding utf8(TextEncoding::Utf8);
        const TextEncoding latin1(TextEncoding::Latin1);
        EncodingSelectionAspect aspect(nullptr);
        aspect.setValue(utf8);
        const std::unique_ptr<QWidget> first(Layouting::Column{aspect}.emerge());
        const std::unique_ptr<QWidget> second(Layouting::Column{aspect}.emerge());
        CodecChooser *firstChooser = first->findChild<CodecChooser *>();
        CodecChooser *secondChooser = second->findChild<CodecChooser *>();
        QVERIFY(firstChooser);
        QVERIFY(secondChooser);

        aspect.setValue(latin1);
        QCOMPARE(firstChooser->currentEncoding(), latin1);
        QCOMPARE(secondChooser->currentEncoding(), latin1);

        firstChooser->setAssignedEncoding(utf8);
        QCOMPARE(aspect.volatileValue(), utf8.name());
        QCOMPARE(secondChooser->currentEncoding(), utf8);
    }
};

QObject *createEncodingSelectionAspectTest()
{
    return new EncodingSelectionAspectTest;
}

} // namespace Internal
#endif // WITH_TESTS

} // TextEditor

#ifdef WITH_TESTS
#include <extraencodingsettings.moc>
#endif
