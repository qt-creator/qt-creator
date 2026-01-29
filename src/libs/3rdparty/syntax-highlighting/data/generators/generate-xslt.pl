#!/usr/bin/perl
# SPDX-FileCopyrightText: Jonathan Poelen <jonathan.poelen@gmail.com>
# SPDX-License-Identifier: MIT

$xslt_base_filename = $0;
$xslt_base_filename =~ s#/[^/]+$#/xslt.base.xml#;

sub readall
{
    open(my $in, '<:encoding(UTF-8)', $_[0])
        or die "Could not open file '$_[0]': $!";

    my $text = join('', <$in>);

    close $in;

    return $text
}

my $xml = readall($ARGV[0]);
my $xslt = readall($xslt_base_filename);

open(my $output, '>:encoding(UTF-8)', $ARGV[1])
  or die "Could not open file '$ARGV[1]': $!";


sub replace_rule
{
    my $kw = $_[0];
    my $sp = $_[1];
    my $rule = $_[2];
    my $ori = $rule;

    if ($kw eq 'keytags')
    {
        if (not $rule =~ /context="El End"/)
        {
            $rule =~ s/context="((?:#pop)*?!??)Element"/context="$1XSLT_Element"/
                or die "XSLT: keytags: unknown context attribute: $ori";
        }

        if ($rule =~ s/^<StringDetect/<keyword/)
        {
            $rule  =~ s/dynamic="[^"]+"//;
            $rule  =~ s#/>$# weakDeliminator="-:."/>#;
            my $rule2 = $rule;
            $rule  =~ s/attribute="Element"/attribute="XSLT Tag"/;
            $rule2 =~ s/attribute="Element"/attribute="XSLT 2.0 Tag"/;
            $rule  =~ s/String="[^"]+"/String="keytags"/;
            $rule2 =~ s/String="[^"]+"/String="keytags_2.0"/;
            return $rule . $sp . $rule2 . $sp . $ori
        }
        elsif ($rule =~ /^<RegExpr/)
        {
            $rule =~ /attribute="Error"/ or die "The Perl script must be updated";
            $rule =~ s/&name;/&xsl_tags;/
                or die "XSLT: keytags: '&name;' not found: $ori";
            return $rule . $sp . $ori
        }
    }
    elsif ($kw eq 'xattr')
    {
        $rule =~ s/context="((?:#pop)*?!??)Attribute"/context="$1XSLT_Attribute"/
            or die "XSLT: xattr: unknown context attribute: $ori";
        $rule =~ s/&name;/&xsl_attrs;/
            or die "XSLT: xattr: '&name;' not found: $ori";
        $rule =~ s/attribute="Attribute"/attribute="XSLT XPath Attribute"/;
        return $rule . $sp . $ori
    }
    elsif ($kw eq 'xvalue')
    {
        $rule =~ s/context="((?:#pop)*?!??)Value ([SD]Q)"/context="$1xpath$2"/
            or die "XSLT: xvalue: unknown context attribute: $ori";
        return $rule
    }
    elsif ($kw eq 'xpathDQ')
    {
        return '<DetectChar attribute="XPath" context="xpathDQ" char="{"/>' . $sp . $ori
    }
    elsif ($kw eq 'xpathSQ')
    {
        return '<DetectChar attribute="XPath" context="xpathSQ" char="{"/>' . $sp . $ori
    }

    die "Unknown keyword: '$kw\'";
}

sub update_copied_rule
{
    my $kw = $_[0];
    my $sp = $_[1];
    my $rule = $_[2];

    if ($kw eq '')
    {
        $rule =~ s/context="((?:#pop)*+!?+)([^"]+)"/context="$1XSLT_$2"/;
        return $rule
    }
    elsif ($kw eq 'nochange')
    {
        return $rule
    }
    else
    {
        return replace_rule($kw, $sp, $rule)
    }
}

# "Value" is named "Attribute Value" in xslt
sub replace_attribute_value
{
    return $_[0] =~ s/attribute="Value"/attribute="Attribute Value"/gr;
}

sub dup_context
{
    my $ctx = replace_attribute_value($_[0]);
    # duplicate with a XSLT name
    my $xslt_ctx = ($ctx =~ s/name="([^"]+)"/name="XSLT_$1"/r);
    # "Attribute Value" becomes "XPath"
    $xslt_ctx =~ s/attribute="Attribute Value"/attribute="XPath"/g;
    # rule preceded by an optional XSLT comment
    # [A-Zk]: select <{rule}>, but not <context>
    $xslt_ctx =~ s#(?:<!-- XSLT: (\w+) -->(\s*))?(<[A-Zk].*?/>)#update_copied_rule($1, $2, $3)#gse;
    # remove XSLT comment from original context
    $ctx =~ s#<!-- XSLT: \w+ -->\s*##gs;
    return $ctx . $xslt_ctx;
}

sub keywords_to_reg
{
    $_[0] =~ /<list name="$_[1]">(.*?)<\/list>/s
        or die "'$_[1]' list not found";
    my $keytags = $1;
    $keytags =~ s#\s*<item>#|#g;
    $keytags =~ s#^\||</item>|\s*##g;
    return $keytags
}

sub get_license
{
    $_[0] =~ /license="([^"]+)"/;
    return $1
}

# check licenses
$xml_license = get_license($xml);
$xslt_license = get_license($xslt);
if ($xml_license ne $xslt_license)
{
    print STDERR "Lisenses xml=$xml_license differ to xslt=$xslt_license";
    exit 1;
}

# update version
$xml =~ /<language.*?version="([^"]+)"/s;
$version = $1 - 25;
$xslt =~ s/(<language.*?version=")([^"]+)/$version += $2 ; "$1$version"/e;

# add xml author
if ($xml =~ /author="([^"]+)"/)
{
    $xml_author = $1;
    $xslt =~ s/author="([^"]+)"/author="$1;$xml_author"/;
}

# extract some <list>
$keytags = keywords_to_reg($xslt, 'keytags');
$keytags2 = keywords_to_reg($xslt, 'keytags_2.0');
$xsl_attrs = keywords_to_reg($xslt, 'xsl-attributes');

# extract xml entities
$xml =~ /(<!ENTITY.*?)\]>/s
    or die "xml 'ENTITY' not found";
$xml_entities = $1;

# inject <ENTITY>
$xslt =~ s#(\s*<!ENTITY)#
  $xml_entities
  <!ENTITY xsl_tags "($keytags|$keytags)(?![\\w.:_-])">
  <!ENTITY xsl_attrs "($xsl_attrs)(?![\\w.:_-])">$1#s;

# extract xml contexts
$xml =~ /<contexts>(.*?)<\/contexts>/s;
$xml_contexts = $1;

# update xml contexts
$xml_contexts =~ s#<!-- XSLT: attrvalue -->(.*?</context>)#replace_attribute_value($1)#gse;
$xml_contexts =~ s#<!-- XSLT: copy -->(.*?</context>)#dup_context($1)#gse;
$xml_contexts =~ s#<!-- XSLT: (\w+) -->(\s*)(<.*?/>)#replace_rule($1, $2, $3)#gse;

# rename first context
$xml_contexts =~ s/name="Start"/name="normalText"/;

# rename some attributes
$xml_contexts =~ s/attribute="Error"/attribute="Invalid"/g;
$xml_contexts =~ s/attribute="Element( Symbols)?"/attribute="Tag$1"/g;
$xml_contexts =~ s/attribute="(P?)EntityRef"/attribute="$1Entity Reference"/g;

# inject xml contexts
$xslt =~ s#(<contexts>\s*)#$1$xml_contexts\n#;

# insert warning
$xslt =~ s#>#>\n\n<!-- ***** THIS FILE WAS GENERATED BY A SCRIPT - DO NOT EDIT ***** -->\n#;

# print xslt syntax
print $output $xslt;
