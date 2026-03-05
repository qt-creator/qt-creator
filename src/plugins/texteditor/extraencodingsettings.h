// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "texteditor_global.h"

#include <utils/store.h>
#include <utils/aspects.h>
#include <utils/textcodec.h>

namespace TextEditor {

class CodecChooser;

class TEXTEDITOR_EXPORT ExtraEncodingSettingsData
{
public:
    enum Utf8BomSetting {
        AlwaysAdd = 0,
        OnlyKeep = 1,
        AlwaysDelete = 2
    };
    Utf8BomSetting m_utf8BomSetting = OnlyKeep;

    enum LineEndingSetting {
      Unix = 0,
      Windows = 1
    };
    LineEndingSetting m_lineEndingSetting = Unix;
};

class TEXTEDITOR_EXPORT EncodingSelectionAspect : public Utils::ByteArrayAspect
{
public:
    explicit EncodingSelectionAspect(Utils::AspectContainer *container);

    Utils::TextEncoding operator()() const;
    void setValue(const Utils::TextEncoding &value);

private:
    void addToLayoutImpl(Layouting::Layout &parent);

    CodecChooser *m_codecChooser = nullptr;
};

class TEXTEDITOR_EXPORT ExtraEncodingSettings : public Utils::AspectContainer
{
public:
    ExtraEncodingSettings();

    void apply() final;

    void setData(const ExtraEncodingSettingsData &data);
    ExtraEncodingSettingsData data() const;

    EncodingSelectionAspect defaultEncoding{this};
    Utils::TypedSelectionAspect<ExtraEncodingSettingsData::Utf8BomSetting> utf8BomSetting{this};
    Utils::TypedSelectionAspect<ExtraEncodingSettingsData::LineEndingSetting> lineEndingSetting{this};
};

void setupExtraEncodingSettings();

TEXTEDITOR_EXPORT ExtraEncodingSettings &globalExtraEncodingSettings();

} // TextEditor
