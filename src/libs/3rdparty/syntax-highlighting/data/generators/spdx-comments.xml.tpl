<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE language SYSTEM "language.dtd">
<language
    version="4"
    kateversion="3.1"
    name="SPDX-Comments"
    section="Other"
    extensions=""
    mimetype=""
    author="Alex Turbov (i.zaufi@gmail.com)"
    license="MIT"
    hidden="true"
  >
  <highlighting>
    <list name="tags">
      <item>SPDX-License-Identifier:</item>
      <item>SPDX-FileContributor:</item>
      <item>SPDX-FileCopyrightText:</item>
      <item>SPDX-LicenseInfoInFile:</item>
    </list>

    <list name="operators">
      <item>AND</item>
      <item>OR</item>
      <item>WITH</item>
    </list>

    <list name="licenses">
      <!--[- for license in licenses if not license.isDeprecatedLicenseId ]-->
      <item><!--{ license.licenseId }--></item>
      <!--[- endfor ]-->
    </list>

    <list name="deprecated-licenses">
      <!--[- for license in licenses if license.isDeprecatedLicenseId ]-->
      <item><!--{ license.licenseId }--></item>
      <!--[- endfor ]-->
    </list>

    <list name="exceptions">
      <!--[- for exception in exceptions if not exception.isDeprecatedLicenseId ]-->
      <item><!--{ exception.licenseExceptionId }--></item>
      <!--[- endfor ]-->
    </list>

    <list name="deprecated-exceptions">
      <!--[- for exception in exceptions if exception.isDeprecatedLicenseId ]-->
      <item><!--{ exception.licenseExceptionId }--></item>
      <!--[- endfor ]-->
    </list>

    <contexts>

      <context name="Normal" attribute="SPDX Tag" lineEndContext="#pop">
        <WordDetect String="SPDX-License-Identifier:" attribute="SPDX Tag" context="license-expression" />
        <keyword String="tags" attribute="SPDX Tag" />
      </context>

      <context name="license-expression" attribute="SPDX Value" lineEndContext="#pop" fallthrough="true" fallthroughContext="#pop">
        <DetectSpaces/>
        <DetectChar char="(" context="#stay" attribute="SPDX License Expression Operator" />
        <DetectChar char=")" context="#stay" attribute="SPDX License Expression Operator" />
        <DetectChar char="+" context="#stay" attribute="SPDX License Expression Operator" />
        <keyword String="licenses" context="#stay" attribute="SPDX License" />
        <keyword String="deprecated-licenses" context="#stay" attribute="SPDX Deprecated License" />
        <keyword String="exceptions" context="#stay" attribute="SPDX License Exception" />
        <keyword String="deprecated-exceptions" context="#stay" attribute="SPDX Deprecated License Exception" />
        <keyword String="operators" context="#stay" attribute="SPDX License Expression Operator" />
        <RegExpr attribute="SPDX License" context="#stay" String="\bLicenseRef-[^\s]+" />
      </context>

    </contexts>

    <itemDatas>
      <itemData name="SPDX Tag" defStyleNum="dsAnnotation" italic="true" spellChecking="false" />
      <itemData name="SPDX Value" defStyleNum="dsAnnotation" italic="true" spellChecking="false" />
      <itemData name="SPDX License" defStyleNum="dsAnnotation" italic="true" spellChecking="false" />
      <itemData name="SPDX License Exception" defStyleNum="dsAnnotation" italic="true" spellChecking="false" />
      <itemData name="SPDX Deprecated License" defStyleNum="dsAnnotation" italic="true" spellChecking="false" />
      <itemData name="SPDX Deprecated License Exception" defStyleNum="dsAnnotation" italic="true" spellChecking="false" />
      <itemData name="SPDX License Expression Operator" defStyleNum="dsOperator" italic="true" spellChecking="false" />
    </itemDatas>

  </highlighting>

  <general>
    <keywords casesensitive="1" weakDeliminator=":-." />
  </general>

</language>
<!-- kate: indent-width 2; -->
