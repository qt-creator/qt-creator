// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "extraencodingsettings.h"

#include "texteditortr.h"
#include "texteditorsettings.h"
#include "codecchooser.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>

#include <utils/layoutbuilder.h>
#include <utils/qtcsettings.h>
#include <utils/textcodec.h>

using namespace Utils;

namespace TextEditor {

EncodingSelectionAspect::EncodingSelectionAspect(AspectContainer *container)
    : TypedAspect<QByteArray>(container)
{}

void EncodingSelectionAspect::addToLayoutImpl(Layouting::Layout &parent)
{
    m_codecChooser = createSubWidget<CodecChooser>();
    m_codecChooser->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    m_codecChooser->setAssignedEncoding(value());
    parent.addItem(m_codecChooser);

    connect(m_codecChooser, &CodecChooser::encodingChanged, this, [this] {
        setVolatileValue(m_codecChooser->currentEncoding().name());
    });
}

void EncodingSelectionAspect::setValue(const TextEncoding &encoding)
{
    TypedAspect<QByteArray>::setValue(encoding.name());
    if (m_codecChooser)
        m_codecChooser->setAssignedEncoding(encoding);
}

TextEncoding EncodingSelectionAspect::operator()() const
{
    return TextEncoding(TypedAspect<QByteArray>::value());
}

ExtraEncodingSettings::ExtraEncodingSettings()
{
    setAutoApply(false);
    setSettingsGroup("textEditorManager");

    defaultEncoding.setSettingsKey(Core::Constants::SETTINGS_DEFAULTTEXTENCODING);
    defaultEncoding.setLabelText(Tr::tr("Default encoding:"));
    defaultEncoding.setDefaultValue(Core::EditorManager::defaultTextEncoding().name());

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

} // TextEditor
