#!/usr/bin/env ruby
#
# Generates the keyword lists for directives and variables used in nginx and
# prints them to stdout, ready to be copy & pasted into nginx.xml.
#
# SPDX-FileCopyrightText: 2023 Jyrki Gadinger <nilsding@nilsding.org>
# SPDX-License-Identifier: MIT
#
# Usage:
#     % ./generate-nginx-lists.rb
#
# if you want to install the required dependencies provide `INSTALL_GEMS` in
# your ENV:
#     % INSTALL_GEMS=1 ./generate-nginx-lists.rb

require "bundler/inline"

gemfile(ENV["INSTALL_GEMS"]) do
  source "https://rubygems.org"

  gem "nokogiri", "~> 1.14"
  gem "faraday", "~> 2.7"
  gem "builder", "~> 3.2"
end

def fetch_vars(url)
  Faraday.get(url)
    .then { Nokogiri::HTML.parse _1.body }
    .css("div#content a[href]")
    .map(&:text)
    .reject { _1.end_with?("_") } # some vars are just a prefix, ignore those
    .sort
    .uniq
end

def build_xml_list(name, url: nil, items: [])
  builder = Builder::XmlMarkup.new(indent: 2)

  builder.comment! "see #{url} for a full list of #{name}"
  builder.list(name:) do |b|
    items.each do |item|
      b.item item
    end
  end
end

{
  directives: "https://nginx.org/en/docs/dirindex.html",
  variables:  "https://nginx.org/en/docs/varindex.html",
}.each do |name, url|
  items = fetch_vars(url)

  puts build_xml_list(name, url:, items:).gsub(/^/, ' ' * 4)
  puts
end
