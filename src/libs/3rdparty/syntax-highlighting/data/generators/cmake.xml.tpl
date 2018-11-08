<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE language SYSTEM "language.dtd"
[
  <!ENTITY id_re "[_A-Za-z][\-_0-9A-Za-z]*">
]>
<!--
  This file is part of KDE's kate project.

  Copyright 2004 Alexander Neundorf (neundorf@kde.org)
  Copyright 2005 Dominik Haumann (dhdev@gmx.de)
  Copyright 2007,2008,2013,2014 Matthew Woehlke (mw_triad@users.sourceforge.net)
  Copyright 2013-2015,2017-2018 Alex Turbov (i.zaufi@gmail.com)

 **********************************************************************
 * This library is free software; you can redistribute it and/or      *
 * modify it under the terms of the GNU Lesser General Public         *
 * License as published by the Free Software Foundation; either       *
 * version 2 of the License, or (at your option) any later version.   *
 *                                                                    *
 * This library is distributed in the hope that it will be useful,    *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of     *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU  *
 * Lesser General Public License for more details.                    *
 *                                                                    *
 * You should have received a copy of the GNU Lesser General Public   *
 * License along with this library; if not, write to the              *
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,   *
 * Boston, MA  02110-1301, USA.                                       *
 **********************************************************************
 -->

<language
    name="CMake"
    version="11"
    kateversion="2.4"
    section="Other"
    extensions="CMakeLists.txt;*.cmake;*.cmake.in"
    style="CMake"
    mimetype="text/x-cmake"
    author="Alex Turbov (i.zaufi@gmail.com)"
    license="LGPLv2+"
  >
  <highlighting>

    <list name="commands">
    {%- for command in commands %}
        <item>{{command.name}}</item>
    {%- endfor %}
    </list>
    {% for command in commands -%}
      {%- if command.named_args and command.named_args.kw %}
    <list name="{{command.name}}_nargs">
        {%- for arg in command.named_args.kw %}
      <item>{{arg}}</item>
        {%- endfor %}
    </list>
      {%- endif %}
      {%- if command.special_args and command.special_args.kw %}
    <list name="{{command.name}}_sargs">
        {%- for arg in command.special_args.kw %}
      <item>{{arg}}</item>
        {%- endfor %}
    </list>
      {%- endif %}
    {%- endfor %}

    <list name="variables">
    {%- for var in variables.kw %}
      <item>{{var}}</item>
    {%- endfor %}
    </list>

    {%- for kind in properties.kinds %}
    <list name="{{ kind|replace('_', '-') }}">
      {%- for prop in properties[kind].kw %}
      <item>{{prop}}</item>
      {%- endfor %}
    </list>
    {%- endfor %}

    <list name="generator-expressions">
      {%- for expr in generator_expressions %}
      <item>{{ expr }}</item>
      {%- endfor %}
    </list>

    <contexts>

      <context attribute="Normal Text" lineEndContext="#stay" name="Normal Text">
        <DetectSpaces/>
        {% for command in commands -%}
        <WordDetect String="{{command.name}}" insensitive="true" attribute="Command" context="{{command.name}}_ctx" />
        {% endfor -%}
        <RegExpr attribute="Region Marker" context="RST Documentation" String="^#\[(=*)\[\.rst:" column="0" />
        <RegExpr attribute="Comment" context="Bracketed Comment" String="#\[(=*)\[" />
        <DetectChar attribute="Comment" context="Comment" char="#" />
        <DetectIdentifier attribute="User Function/Macro" context="User Function" />
        <RegExpr attribute="@Variable Substitution" context="@VarSubst" String="@&id_re;@" lookAhead="true" />
        <!-- Include keywords matching for language autocompleter work -->
        <keyword attribute="Command" context="#stay" String="commands" />
      </context>

      {% for command in commands -%}
      {#
      <!--
        {{ command|pprint }}
      -->
      -#}
      <context attribute="Normal Text" lineEndContext="#stay" name="{{command.name}}_ctx">
        <DetectChar attribute="Normal Text" context="{{command.name}}_ctx_op" char="(" />
      </context>
      <context attribute="Normal Text" lineEndContext="#stay" name="{{command.name}}_ctx_op">
        {%- if command.nested_parentheses %}
        <DetectChar attribute="Normal Text" context="{{command.name}}_ctx_op_nested" char="(" />
        {%- endif %}
        <IncludeRules context="EndCmdPop2" />
        {%- if command.named_args and command.named_args.kw %}
        <keyword attribute="Named Args" context="#stay" String="{{command.name}}_nargs" />
        {%- endif %}
        {%- if command.special_args and command.special_args.kw %}
        <keyword attribute="Special Args" context="#stay" String="{{command.name}}_sargs" />
        {%- endif %}
        {%- if command.property_args and command.property_args.kw %}
          {%- for kind in command.property_args.kw %}
        <keyword attribute="Property" context="#stay" String="{{kind}}" />
            {%- if properties[kind|replace('-', '_')].re %}
        <IncludeRules context="Detect More {{kind}}" />
            {%- endif %}
          {%- endfor %}
        {%- endif %}
        {%- if command is not nulary %}
        <IncludeRules context="User Function Args" />
          {%- if command.name == 'cmake_policy' %}
        <!-- NOTE Handle CMP<NNN> as a special arg of `cmake_policy` command -->
        <RegExpr attribute="Special Args" context="#stay" String="\bCMP[0-9]+\b" />
          {%- endif %}
        {%- endif %}
      </context>
        {%- if command.nested_parentheses %}
      <context attribute="Normal Text" lineEndContext="#stay" name="{{command.name}}_ctx_op_nested">
        <IncludeRules context="EndCmdPop" />
        {%- if command.named_args and command.named_args.kw %}
        <keyword attribute="Named Args" context="#stay" String="{{command.name}}_nargs" />
        {%- endif %}
        {%- if command.special_args and command.special_args.kw %}
        <keyword attribute="Special Args" context="#stay" String="{{command.name}}_sargs" />
        {%- endif %}
        {%- if command.property_args and command.property_args.kw %}
          {%- for kind in command.property_args.kw %}
        <keyword attribute="Property" context="#stay" String="{{kind}}" />
              {%- if properties[kind|replace('-', '_')].re %}
        <IncludeRules context="Detect More {{kind}}" />
            {%- endif %}
          {%- endfor %}
        {%- endif %}
        <IncludeRules context="User Function Args" />
      </context>
        {%- endif %}
      {% endfor -%}

      {% for kind in properties.kinds if properties[kind].re -%}
      <context attribute="Normal Text" lineEndContext="#stay" name="Detect More {{ kind|replace('_', '-') }}">
        {%- for prop in properties[kind].re %}
        <RegExpr attribute="Property" context="#stay" String="{{prop}}" />
        {%- endfor %}
      </context>{{ '\n' }}
      {% endfor -%}

      <context attribute="Normal Text" lineEndContext="#stay" name="EndCmdPop">
        <DetectChar attribute="Normal Text" context="#pop" char=")" />
      </context>

      <context attribute="Normal Text" lineEndContext="#stay" name="EndCmdPop2">
        <DetectChar attribute="Normal Text" context="#pop#pop" char=")" />
      </context>

      <context attribute="User Function/Macro" lineEndContext="#stay" name="User Function">
        <DetectChar attribute="Normal Text" context="User Function Opened" char="(" />
        <IncludeRules context="EndCmdPop2" />
      </context>

      <context attribute="Normal Text" lineEndContext="#stay" name="User Function Opened">
        <IncludeRules context="EndCmdPop2" />
        <IncludeRules context="User Function Args" />
      </context>

      <context attribute="Normal Text" lineEndContext="#stay" name="Detect Builtin Variables">
        <keyword attribute="Builtin Variable" context="#stay" String="variables" insensitive="false" />
        <IncludeRules context="Detect More Builtin Variables" />
        <RegExpr attribute="Internal Name" context="#stay" String="\b_&id_re;\b" />
      </context>

      <context attribute="Normal Text" lineEndContext="#stay" name="Detect More Builtin Variables">
        {%- for var in variables.re %}
        <RegExpr attribute="Builtin Variable" context="#stay" String="{{var}}" />
        {%- endfor %}
      </context>

      <context attribute="Normal Text" lineEndContext="#stay" name="Detect Variable Substitutions">
        <RegExpr attribute="Environment Variable Substitution" context="#stay" String="\$ENV\{\s*[\w-]+\s*\}" />
        <Detect2Chars attribute="Variable Substitution" context="VarSubst" char="$" char1="{" />
        <RegExpr attribute="@Variable Substitution" context="@VarSubst" String="@&id_re;@" lookAhead="true" />
      </context>

      <context attribute="Variable Substitution" lineEndContext="#pop" name="VarSubst">
        <IncludeRules context="Detect Builtin Variables" />
        <DetectIdentifier />
        <DetectChar attribute="Variable Substitution" context="#pop" char="}" />
        <IncludeRules context="Detect Variable Substitutions" />
      </context>

      <context attribute="@Variable Substitution" lineEndContext="#pop" name="@VarSubst">
        <DetectChar attribute="@Variable Substitution" context="VarSubst@" char="@" />
      </context>

      <context attribute="@Variable Substitution" lineEndContext="#pop#pop" name="VarSubst@">
        <IncludeRules context="Detect Builtin Variables" />
        <DetectIdentifier />
        <DetectChar attribute="@Variable Substitution" context="#pop#pop" char="@" />
      </context>

      <context attribute="Normal Text" lineEndContext="#stay" name="User Function Args">
        <Detect2Chars attribute="Normal Text" context="#stay" char="\" char1="(" />
        <Detect2Chars attribute="Normal Text" context="#stay" char="\" char1=")" />
        <RegExpr attribute="Escapes" context="#stay" String="\\[&quot;$n\\]" />
        <DetectChar attribute="Strings" context="String" char="&quot;" />
        <RegExpr attribute="Strings" context="Bracketed String" String="\[(=*)\[" />
        <RegExpr attribute="Comment" context="Bracketed Comment" String="#\[(=*)\[" />
        <DetectChar attribute="Comment" context="Comment" char="#" />
        <IncludeRules context="Detect Builtin Variables" />
        <IncludeRules context="Detect Variable Substitutions" />
        <IncludeRules context="Detect Special Values" />
        <IncludeRules context="Detect Aliased Targets" />
        <IncludeRules context="Detect Generator Expressions" />
      </context>

      <context attribute="Normal Text" lineEndContext="#stay" name="Detect Special Values">
        <RegExpr attribute="True Special Arg" context="#stay" String="\b(TRUE|ON)\b" />
        <RegExpr attribute="False Special Arg" context="#stay" String="\b(FALSE|OFF|(&id_re;-)?NOTFOUND)\b" />
        <RegExpr attribute="Special Args" context="#stay" String="\bCMP[0-9][0-9][0-9]\b" />
      </context>

      <context attribute="Normal Text" lineEndContext="#stay" name="Detect Aliased Targets">
        <RegExpr attribute="Aliased Targets" context="#stay" String="\b&id_re;::&id_re;(::&id_re;)*\b" />
      </context>

      <context attribute="Comment" lineEndContext="#pop" name="Comment">
        <LineContinue attribute="Comment" context="#pop" />
        <IncludeRules context="##Alerts" />
        <IncludeRules context="##Modelines" />
      </context>

      <context attribute="Comment" lineEndContext="#stay" name="RST Documentation" dynamic="true">
        <RegExpr attribute="Region Marker" context="#pop" String="^#?\]%1\]" dynamic="true" column="0" />
        <IncludeRules context="##reStructuredText" />
      </context>

      <context attribute="Comment" lineEndContext="#stay" name="Bracketed Comment" dynamic="true">
        <RegExpr attribute="Comment" context="#pop" String=".*\]%1\]" dynamic="true" />
        <IncludeRules context="##Alerts" />
        <IncludeRules context="##Modelines" />
      </context>

      <context attribute="Strings" lineEndContext="#stay" name="String">
        <RegExpr attribute="Strings" context="#pop" String="&quot;(?=[ );]|$)" />
        <RegExpr attribute="Escapes" context="#stay" String="\\[&quot;$nrt\\]" />
        <IncludeRules context="Detect Variable Substitutions" />
        <IncludeRules context="Detect Generator Expressions" />
      </context>

      <context attribute="Strings" lineEndContext="#stay" name="Bracketed String" dynamic="true">
        <RegExpr attribute="Strings" context="#pop" String="\]%1\]" dynamic="true" />
      </context>

      <context attribute="Normal Text" lineEndContext="#stay" name="Detect Generator Expressions">
        <Detect2Chars attribute="Generator Expression" context="Generator Expression" char="$" char1="&lt;" />
      </context>

      <context attribute="Generator Expression" lineEndContext="#stay" name="Generator Expression">
        <IncludeRules context="Detect Generator Expressions" />
        <DetectChar attribute="Comment" context="Comment" char="#" />
        <DetectChar attribute="Generator Expression" context="#pop" char="&gt;" />
        <keyword attribute="Generator Expression Keyword" context="#stay" String="generator-expressions" insensitive="false" />
        <IncludeRules context="Detect Aliased Targets" />
        <IncludeRules context="Detect Variable Substitutions" />
      </context>

    </contexts>

    <itemDatas>
      <itemData name="Normal Text" defStyleNum="dsNormal" spellChecking="false" />
      <itemData name="Command" defStyleNum="dsKeyword" spellChecking="false" />
      <itemData name="User Function/Macro"  defStyleNum="dsFunction" spellChecking="false" />
      <itemData name="Property" defStyleNum="dsOthers" spellChecking="false" />
      <itemData name="Aliased Targets" defStyleNum="dsBaseN" spellChecking="false" />
      <itemData name="Named Args" defStyleNum="dsOthers" spellChecking="false" />
      <itemData name="Special Args" defStyleNum="dsOthers" spellChecking="false" />
      <itemData name="True Special Arg" defStyleNum="dsOthers" color="#30a030" selColor="#30a030" spellChecking="false" />
      <itemData name="False Special Arg" defStyleNum="dsOthers" color="#e05050" selColor="#e05050" spellChecking="false" />
      <itemData name="Strings" defStyleNum="dsString" spellChecking="true" />
      <itemData name="Escapes" defStyleNum="dsChar" spellChecking="false" />
      <itemData name="Builtin Variable" defStyleNum="dsDecVal" color="#c09050" selColor="#c09050" spellChecking="false" />
      <itemData name="Variable Substitution" defStyleNum="dsDecVal" spellChecking="false" />
      <itemData name="@Variable Substitution" defStyleNum="dsBaseN" spellChecking="false" />
      <itemData name="Internal Name" defStyleNum="dsDecVal" color="#303030" selColor="#303030" spellChecking="false" />
      <itemData name="Environment Variable Substitution" defStyleNum="dsFloat" spellChecking="false" />
      <itemData name="Generator Expression Keyword" defStyleNum="dsKeyword" color="#b84040" selColor="#b84040" spellChecking="false" />
      <itemData name="Generator Expression" defStyleNum="dsOthers" color="#b86050" selColor="#b86050" spellChecking="false" />
      <itemData name="Comment" defStyleNum="dsComment" spellChecking="true" />
      <itemData name="Region Marker" defStyleNum="dsRegionMarker" spellChecking="false" />
    </itemDatas>

  </highlighting>

  <general>
    <comments>
      <comment name="singleLine" start="#" />
    </comments>
    <keywords casesensitive="1" />
  </general>
</language>

<!-- kate: indent-width 2; tab-width 2; -->
