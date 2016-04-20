// Copyright (c) 2007 Roberto Raggi <roberto.raggi@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of
// this software and associated documentation files (the "Software"), to deal in
// the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
// the Software, and to permit persons to whom the Software is furnished to do so,
// subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
// FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
// IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

// ### TODO: Rewrite me.

#include <string>
#include <iostream>
#include <map>
#include <list>
#include <vector>
#include <set>
#include <cstdlib>
#include <cctype>
#include <functional>
#include <algorithm>

class State;
class DottedItem;

typedef std::list<std::string> RuleList;
typedef RuleList::iterator RulePtr;
typedef std::list<State> StateList;
typedef StateList::iterator StatePtr;
typedef std::string::iterator Dot;
typedef std::vector<DottedItem>::iterator DottedItemPtr;

class DottedItem {
public:
  RulePtr rule;
  Dot dot;

  DottedItem() {}

  DottedItem(RulePtr rule, Dot dot):
    rule(rule), dot(dot) {}

  bool operator == (const DottedItem &other) const {
    return rule == other.rule && dot == other.dot;
  }

  bool operator != (const DottedItem &other) const {
    return ! operator == (other);
  }

  bool terminal() const {
    return dot == rule->end();
  }

  DottedItem next() const {
    DottedItem item;
    item.rule = rule;
    item.dot = dot;
    ++item.dot;
    return item;
  }
};

class State {
public:
  State() {}

  template <typename _ForwardIterator>
  State(_ForwardIterator first, _ForwardIterator last) {
    _items.insert(_items.end(), first, last);
  }

  static State &intern(const State &state) {
    StatePtr ptr = std::find(first_state(), last_state(), state);
    if (ptr == last_state())
      ptr = states().insert(last_state(), state);
    return *ptr;
  }

  State &next(char ch) {
    std::vector<DottedItem> n;
    for (DottedItemPtr it = first_item(); it != last_item(); ++it) {
      if (! it->terminal() && *it->dot == ch)
        n.push_back(it->next());
    }
    return intern(State(n.begin(), n.end()));
  }

  std::set<char> firsts() {
    std::set<char> s;
    for (DottedItemPtr it = first_item(); it != last_item(); ++it) {
      if (! it->terminal())
        s.insert(*it->dot);
    }
    return s;
  }

  size_t item_count() const { return _items.size(); }

  DottedItemPtr first_item() { return _items.begin(); }
  DottedItemPtr last_item() { return _items.end(); }

  static StatePtr first_state() { return states().begin(); }
  static StatePtr last_state() { return states().end(); }

  bool operator == (const State &other) const { return _items == other._items; }
  bool operator != (const State &other) const { return _items != other._items; }

  template <typename _Iterator>
  static State &start(_Iterator first, _Iterator last) {
    std::vector<DottedItem> items;
    for (; first != last; ++first)
      items.push_back(DottedItem(first, first->begin()));
    return intern(State(items.begin(), items.end()));
  }

  static void reset() {
    states().clear();
  }

private:
  static StateList &states() {
    static StateList _states;
    return _states;
  }

private:
  std::vector<DottedItem> _items;
};

static bool option_no_enums = false;
static bool option_toupper = false;
static std::string option_namespace_name;
static std::string option_token_prefix = "Token_";
static std::string option_char_type = "char";
static std::string option_unicode_function = "";

std::string token_id(const std::string &id)
{
  std::string token = option_token_prefix;

  if (! option_toupper)
    token += id;
  else {
    for (size_t i = 0; i < id.size(); ++i)
      token += toupper(id[i]);
  }

  return token;
}

bool starts_with(const std::string &line, const std::string &text) {
  if (text.length() < line.length()) {
    return std::equal(line.begin(), line.begin() + text.size(), text.begin());
  }
  return false;
}

void doit(State &state)
{
  static int depth{0};

  ++depth;

  std::string indent(depth * 2, ' ');

  std::set<char> firsts = state.firsts();
  for (std::set<char>::iterator it = firsts.begin(); it != firsts.end(); ++it) {
    std::string _else = it == firsts.begin() ? "" : "else ";
    std::cout << indent << _else << "if (s[" << (depth - 1) << "]" << option_unicode_function << " == '" << *it << "') {" << std::endl;
    State &next_state = state.next(*it);

    bool found = false;
    for (DottedItemPtr item = next_state.first_item(); item != next_state.last_item(); ++item) {
      if (item->terminal()) {
	if (found) {
	  std::cerr << "*** Error. Too many accepting states" << std::endl;
	  exit(EXIT_FAILURE);
	}
	found = true;
	std::cout << indent << "  return " << option_namespace_name << token_id(*item->rule) << ";" << std::endl;
      }
    }

    if (! found)
      doit(next_state);

    std::cout << indent << "}" << std::endl;
  }

  --depth;
}

void gen_classify_n(State &start_state, int N)
{
  std::cout << "static inline int classify" << N << "(const " << option_char_type << " *s) {" << std::endl;
  doit(start_state);
  std::cout << "  return " << option_namespace_name << token_id("identifier") << ";" << std::endl
            << "}" << std::endl << std::endl;
}

void gen_classify(const std::multimap<size_t, std::string> &keywords)
{
  std::cout << "int " << option_namespace_name << "classify(const " << option_char_type << " *s, int n) {" << std::endl
            << "  switch (n) {" << std::endl;
  std::multimap<size_t, std::string>::const_iterator it = keywords.begin();
  while (it != keywords.end()) {
    size_t size = it->first;
    std::cout << "    case " << size << ": return classify" << size << "(s);" << std::endl;
    do { ++it; } while (it != keywords.end() && it->first == size);
  }
  std::cout << "    default: return " << option_namespace_name << token_id("identifier") << ";" << std::endl
            << "  } // switch" << std::endl
            << "}" << std::endl << std::endl;
}

void gen_enums(const std::multimap<size_t, std::string> &keywords)
{
  std::cout << "enum {" << std::endl;
  std::multimap<size_t, std::string>::const_iterator it = keywords.begin();
  for (; it != keywords.end(); ++it) {
    std::cout << "  " << token_id(it->second) << "," << std::endl;
  }
  std::cout << "  " << token_id("identifier") << std::endl
            << "};" << std::endl << std::endl;
}

inline bool not_whitespace_p(char ch) {
  return ! std::isspace(ch);
}

int main(int argc, char *argv[]) {
  const std::string ns = "--namespace=";

  for (int i = 0; i < argc; ++i) {
    const std::string arg(argv[i]);
    if (arg == "--no-enums")
      option_no_enums = true;
    else if (starts_with(arg, ns)) {
      option_namespace_name.assign(arg.begin() + ns.size(), arg.end());
      option_namespace_name += "::";
    }
  }

  std::multimap<size_t, std::string> keywords;
  std::string textline;

  bool readKeywords = false;

  const std::string opt_no_enums = "%no-enums";
  const std::string opt_toupper = "%toupper";
  const std::string opt_ns = "%namespace=";
  const std::string opt_tok_prefix = "%token-prefix=";
  const std::string opt_char_type = "%char-type=";
  const std::string opt_unicode_function = "%unicode-function=";

  while (getline(std::cin, textline)) {

    // remove trailing spaces
    textline.assign(textline.begin(), std::find_if(textline.rbegin(), textline.rend(), not_whitespace_p).base());

    if (! readKeywords) {
      if (textline.size() >= 2 && textline[0] == '%') {
	if (textline[1] == '%') {
	  readKeywords = true;
	} else if (textline == opt_no_enums) {
	  option_no_enums = true;
	} else if (textline == opt_toupper) {
	  option_toupper = true;
	} else if (starts_with(textline, opt_tok_prefix)) {
	  option_token_prefix.assign(textline.begin() + opt_tok_prefix.size(), textline.end());
	} else if (starts_with(textline, opt_char_type)) {
	  option_char_type.assign(textline.begin() + opt_char_type.size(), textline.end());
	} else if (starts_with(textline, opt_unicode_function)) {
	  option_unicode_function.assign(textline.begin() + opt_unicode_function.size(), textline.end());
	} else if (starts_with(textline, opt_ns)) {
	  option_namespace_name.assign(textline.begin() + opt_ns.size(), textline.end());
	  option_namespace_name += "::";
	}

	continue;
      }
      std::cout << textline << std::endl;
    } else {
      if (textline.empty())
	continue;

      std::string::iterator start = textline.begin();
      while (start != textline.end() && std::isspace(*start))
	++start;

      std::string::iterator stop = start;
      while (stop != textline.end() && (std::isalnum(*stop) || *stop == '_'))
	++stop;

      if (start != stop) {
	std::string keyword(start, stop);
	if (keyword == "identifier") {
	  std::cerr << "*** Error. `identifier' is reserved" << std::endl;
	  exit(EXIT_FAILURE);
	}

	keywords.insert(std::make_pair(keyword.size(), keyword));
      }
    }
  }

  if (! option_no_enums)
    gen_enums(keywords);

  std::multimap<size_t, std::string>::iterator it = keywords.begin();
  while (it != keywords.end()) {
    size_t size = it->first;
    RuleList rules;
    do { rules.push_back(it->second); ++it; }
    while (it != keywords.end() && it->first == size);
    gen_classify_n(State::start(rules.begin(), rules.end()), size);
    State::reset();
  }

  gen_classify(keywords);
}
