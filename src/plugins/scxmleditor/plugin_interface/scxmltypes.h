/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#pragma once

#include <QVariant>
#include <QVector>

namespace ScxmlEditor {

namespace PluginInterface {

enum TagType {
    UnknownTag = 0,
    Metadata,
    MetadataItem,
    Scxml,
    State,
    Parallel,
    Transition,
    InitialTransition,
    Initial,
    Final,
    OnEntry,
    OnExit,
    History,
    Raise,
    If,
    ElseIf,
    Else,
    Foreach,
    Log,
    DataModel,
    Data,
    Assign,
    Donedata,
    Content,
    Param,
    Script,
    Send,
    Cancel,
    Invoke,
    Finalize
};

struct scxmltag_attribute_t;
struct scxmltag_type_t;

struct scxmltag_attribute_t
{
    const char *name; // scxml attribute name
    const char *value; // default value
    bool required;
    bool editable;
    int datatype;
};

struct scxmltag_type_t
{
    const char *name; // scxml output name
    bool canIncludeContent;
    const scxmltag_attribute_t *attributes;
    int n_attributes;
};

// Define tag-attributes
const scxmltag_attribute_t scxml_scxml_attributes[] = {
    {"initial", nullptr, false, false, QVariant::String},
    {"name", nullptr, false, true, QVariant::String},
    {"xmlns", "http://www.w3.org/2005/07/scxml", true, false, QVariant::String},
    {"version", "1.0", true, false, QVariant::String},
    {"datamodel", nullptr, false, true, QVariant::String},
    {"binding", "early;late", false, true, QVariant::StringList}
};

const scxmltag_attribute_t scxml_state_attributes[] = {
    {"id", nullptr, false, true, QVariant::String},
    {"initial", nullptr, false, false, QVariant::String}
};

const scxmltag_attribute_t scxml_parallel_attributes[] = {
    {"id", nullptr, false, true, QVariant::String}
};

const scxmltag_attribute_t scxml_transition_attributes[] = {
    {"event", nullptr, false, true, QVariant::String},
    {"cond", nullptr, false, true, QVariant::String},
    {"target", nullptr, false, true, QVariant::String},
    {"type", "internal;external", false, true, QVariant::StringList}
};

const scxmltag_attribute_t scxml_initialtransition_attributes[] = {
    {"target", nullptr, false, false, QVariant::String}
};

const scxmltag_attribute_t scxml_final_attributes[] = {
    {"id", nullptr, false, true, QVariant::String}
};

const scxmltag_attribute_t scxml_history_attributes[] = {
    {"id", nullptr, false, true, QVariant::String},
    {"type", "shallow;deep", false, true, QVariant::StringList}
};

const scxmltag_attribute_t scxml_raise_attributes[] = {
    {"event", nullptr, true, true, QVariant::String}
};

const scxmltag_attribute_t scxml_if_attributes[] = {
    {"cond", nullptr, true, true, QVariant::String},
};

const scxmltag_attribute_t scxml_elseif_attributes[] = {
    {"cond", nullptr, true, true, QVariant::String}
};

const scxmltag_attribute_t scxml_foreach_attributes[] = {
    {"array", nullptr, true, true, QVariant::String},
    {"item", nullptr, true, true, QVariant::String},
    {"index", nullptr, false, true, QVariant::String}
};

const scxmltag_attribute_t scxml_log_attributes[] = {
    {"label", "", false, true, QVariant::String},
    {"expr", nullptr, false, true, QVariant::String}
};

const scxmltag_attribute_t scxml_data_attributes[] = {
    {"id", nullptr, true, true, QVariant::String},
    {"src", nullptr, false, true, QVariant::String},
    {"expr", nullptr, false, true, QVariant::String}
};

const scxmltag_attribute_t scxml_assign_attributes[] = {
    {"location", nullptr, true, true, QVariant::String},
    {"expr", nullptr, false, true, QVariant::String}
};

const scxmltag_attribute_t scxml_content_attributes[] = {
    {"expr", nullptr, false, true, QVariant::String}
};

const scxmltag_attribute_t scxml_param_attributes[] = {
    {"name", nullptr, true, true, QVariant::String},
    {"expr", nullptr, false, true, QVariant::String},
    {"location", nullptr, false, true, QVariant::String}
};

const scxmltag_attribute_t scxml_script_attributes[] = {
    {"src", nullptr, false, true, QVariant::String}
};

const scxmltag_attribute_t scxml_send_attributes[] = {
    {"event", nullptr, false, true, QVariant::String},
    {"eventexpr", nullptr, false, true, QVariant::String},
    {"target", nullptr, false, true, QVariant::String},
    {"targetexpr", nullptr, false, true, QVariant::String},
    {"type", nullptr, false, true, QVariant::String},
    {"typeexpr", nullptr, false, true, QVariant::String},
    {"id", nullptr, false, true, QVariant::String},
    {"idlocation", nullptr, false, true, QVariant::String},
    {"delay", nullptr, false, true, QVariant::String},
    {"delayexpr", nullptr, false, true, QVariant::String},
    {"namelist", nullptr, false, true, QVariant::String}
};

const scxmltag_attribute_t scxml_cancel_attributes[] = {
    {"sendid", nullptr, false, true, QVariant::String},
    {"sendidexpr", nullptr, false, true, QVariant::String}
};

const scxmltag_attribute_t scxml_invoke_attributes[] = {
    {"type", nullptr, false, true, QVariant::String},
    {"typeexpr", nullptr, false, true, QVariant::String},
    {"src", nullptr, false, true, QVariant::String},
    {"srcexpr", nullptr, false, true, QVariant::String},
    {"id", nullptr, false, true, QVariant::String},
    {"idlocation", nullptr, false, true, QVariant::String},
    {"namelist", nullptr, false, true, QVariant::String},
    {"autoforward", ";true;false", false, true, QVariant::StringList}
};

const scxmltag_type_t scxml_unknown = {
    "unknown",
    true,
    nullptr,
    0
};

const scxmltag_type_t scxml_metadata = {
    "metadata",
    true,
    nullptr,
    0
};

const scxmltag_type_t scxml_metadataitem = {
    "item",
    true,
    nullptr,
    0
};

const scxmltag_type_t scxml_scxml = {
    "scxml",
    false,
    scxml_scxml_attributes,
    sizeof(scxml_scxml_attributes) / sizeof(scxml_scxml_attributes[0])
};

const scxmltag_type_t scxml_state = {
    "state",
    false,
    scxml_state_attributes,
    sizeof(scxml_state_attributes) / sizeof(scxml_state_attributes[0])
};

const scxmltag_type_t scxml_parallel = {
    "parallel",
    false,
    scxml_parallel_attributes,
    sizeof(scxml_parallel_attributes) / sizeof(scxml_parallel_attributes[0])
};

const scxmltag_type_t scxml_transition = {
    "transition",
    false,
    scxml_transition_attributes,
    sizeof(scxml_transition_attributes) / sizeof(scxml_transition_attributes[0])
};

const scxmltag_type_t scxml_initialtransition = {
    "transition",
    false,
    scxml_initialtransition_attributes,
    sizeof(scxml_initialtransition_attributes) / sizeof(scxml_initialtransition_attributes[0])
};

const scxmltag_type_t scxml_initial = {
    "initial",
    false,
    nullptr,
    0
};

const scxmltag_type_t scxml_final = {
    "final",
    false,
    scxml_final_attributes,
    sizeof(scxml_final_attributes) / sizeof(scxml_final_attributes[0])
};

const scxmltag_type_t scxml_onentry = {
    "onentry",
    false,
    nullptr,
    0
};

const scxmltag_type_t scxml_onexit = {
    "onexit",
    false,
    nullptr,
    0
};

const scxmltag_type_t scxml_history = {
    "history",
    false,
    scxml_history_attributes,
    sizeof(scxml_history_attributes) / sizeof(scxml_history_attributes[0])
};

const scxmltag_type_t scxml_raise = {
    "raise",
    false,
    scxml_raise_attributes,
    sizeof(scxml_raise_attributes) / sizeof(scxml_raise_attributes[0])
};

const scxmltag_type_t scxml_if = {
    "if",
    false,
    scxml_if_attributes,
    sizeof(scxml_if_attributes) / sizeof(scxml_if_attributes[0])
};

const scxmltag_type_t scxml_elseif = {
    "elseif",
    false,
    scxml_elseif_attributes,
    sizeof(scxml_elseif_attributes) / sizeof(scxml_elseif_attributes[0])
};

const scxmltag_type_t scxml_else = {
    "else",
    false,
    nullptr,
    0
};

const scxmltag_type_t scxml_foreach = {
    "foreach",
    false,
    scxml_foreach_attributes,
    sizeof(scxml_foreach_attributes) / sizeof(scxml_foreach_attributes[0])
};

const scxmltag_type_t scxml_log = {
    "log",
    false,
    scxml_log_attributes,
    sizeof(scxml_log_attributes) / sizeof(scxml_log_attributes[0])
};

const scxmltag_type_t scxml_datamodel = {
    "datamodel",
    false,
    nullptr,
    0
};

const scxmltag_type_t scxml_data = {
    "data",
    false,
    scxml_data_attributes,
    sizeof(scxml_data_attributes) / sizeof(scxml_data_attributes[0])
};

const scxmltag_type_t scxml_assign = {
    "assign",
    false,
    scxml_assign_attributes,
    sizeof(scxml_assign_attributes) / sizeof(scxml_assign_attributes[0])
};

const scxmltag_type_t scxml_donedata = {
    "donedata",
    false,
    nullptr,
    0
};

const scxmltag_type_t scxml_content = {
    "content",
    false,
    scxml_content_attributes,
    sizeof(scxml_content_attributes) / sizeof(scxml_content_attributes[0])
};

const scxmltag_type_t scxml_param = {
    "param",
    false,
    scxml_param_attributes,
    sizeof(scxml_param_attributes) / sizeof(scxml_param_attributes[0])
};

const scxmltag_type_t scxml_script = {
    "script",
    true,
    scxml_script_attributes,
    sizeof(scxml_script_attributes) / sizeof(scxml_script_attributes[0])
};

const scxmltag_type_t scxml_send = {
    "send",
    false,
    scxml_send_attributes,
    sizeof(scxml_send_attributes) / sizeof(scxml_send_attributes[0])
};

const scxmltag_type_t scxml_cancel = {
    "cancel",
    false,
    scxml_cancel_attributes,
    sizeof(scxml_cancel_attributes) / sizeof(scxml_cancel_attributes[0])
};

const scxmltag_type_t scxml_invoke = {
    "invoke",
    false,
    scxml_invoke_attributes,
    sizeof(scxml_invoke_attributes) / sizeof(scxml_invoke_attributes[0])
};

const scxmltag_type_t scxml_finalize = {
    "finalize",
    false,
    nullptr,
    0
};

const scxmltag_type_t scxml_tags[] = {
    scxml_unknown,
    scxml_metadata,
    scxml_metadataitem,
    scxml_scxml,
    scxml_state,
    scxml_parallel,
    scxml_transition,
    scxml_initialtransition,
    scxml_initial,
    scxml_final,
    scxml_onentry,
    scxml_onexit,
    scxml_history,
    scxml_raise,
    scxml_if,
    scxml_elseif,
    scxml_else,
    scxml_foreach,
    scxml_log,
    scxml_datamodel,
    scxml_data,
    scxml_assign,
    scxml_donedata,
    scxml_content,
    scxml_param,
    scxml_script,
    scxml_send,
    scxml_cancel,
    scxml_invoke,
    scxml_finalize
};

} // namespace PluginInterface
} // namespace ScxmlEditor
