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

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <iostream>
#include <list>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

class State;
class DottedItem;

struct Rule
{
    std::string keyword;
    std::string preCheck;
};

typedef std::list<Rule> RuleList;
typedef RuleList::iterator RulePtr;
typedef std::list<State> StateList;
typedef StateList::iterator StatePtr;
typedef std::string::iterator Dot;
typedef std::vector<DottedItem>::iterator DottedItemPtr;

class DottedItem
{
public:
    RulePtr rule;
    Dot dot;

    DottedItem() {}

    DottedItem(RulePtr rule, Dot dot)
        : rule(rule)
        , dot(dot)
    {}

    bool operator==(const DottedItem &other) const
    {
        return rule == other.rule && dot == other.dot;
    }

    bool operator!=(const DottedItem &other) const { return !operator==(other); }

    bool terminal() const { return dot == rule->keyword.end(); }

    DottedItem next() const
    {
        DottedItem item;
        item.rule = rule;
        item.dot = dot;
        ++item.dot;
        return item;
    }
};

class State
{
public:
    State() {}

    template<typename _ForwardIterator>
    State(_ForwardIterator first, _ForwardIterator last)
    {
        _items.insert(_items.end(), first, last);
    }

    static State &intern(const State &state)
    {
        StatePtr ptr = std::find(first_state(), last_state(), state);
        if (ptr == last_state())
            ptr = states().insert(last_state(), state);
        return *ptr;
    }

    State &next(char ch)
    {
        std::vector<DottedItem> n;
        for (DottedItemPtr it = first_item(); it != last_item(); ++it) {
            if (!it->terminal() && *it->dot == ch)
                n.push_back(it->next());
        }
        return intern(State(n.begin(), n.end()));
    }

    std::vector<char> firsts()
    {
        std::set<char> charsSet;
        for (DottedItemPtr it = first_item(); it != last_item(); ++it) {
            if (!it->terminal())
                charsSet.insert(*it->dot);
        }
        std::vector<char> charsOrderedUpperToBack; // to minimize Keywords.cpp diff
        charsOrderedUpperToBack.reserve(charsSet.size());
        for (char c : charsSet) {
            charsOrderedUpperToBack.push_back(c);
        }
        std::stable_partition(charsOrderedUpperToBack.begin(),
                              charsOrderedUpperToBack.end(),
                              [](char c) {
                                  return !std::isupper(static_cast<unsigned char>(c));
                              });
        return charsOrderedUpperToBack;
    }

    bool hasPreChecks()
    {
        for (DottedItemPtr it = first_item(); it != last_item(); ++it) {
            if (!it->rule->preCheck.empty()) {
                return true;
            }
        }
        return false;
    }

    std::string commonPreCheck(char ch)
    {
        std::string result;
        for (DottedItemPtr it = first_item(); it != last_item(); ++it) {
            if (!it->terminal() && *it->dot == ch) {
                if (result.empty()) {
                    if (it->rule->preCheck.empty()) {
                        return "";
                    }
                    result = (it->rule->preCheck);
                } else if (result != it->rule->preCheck) {
                    return "";
                }
            }
        }
        return result;
    }

    size_t item_count() const { return _items.size(); }

    DottedItemPtr first_item() { return _items.begin(); }
    DottedItemPtr last_item() { return _items.end(); }

    static StatePtr first_state() { return states().begin(); }
    static StatePtr last_state() { return states().end(); }

    bool operator==(const State &other) const { return _items == other._items; }
    bool operator!=(const State &other) const { return _items != other._items; }

    template<typename _Iterator>
    static State &start(_Iterator first, _Iterator last)
    {
        std::vector<DottedItem> items;
        for (; first != last; ++first)
            items.push_back(DottedItem(first, first->keyword.begin()));
        return intern(State(items.begin(), items.end()));
    }

    static void reset() { states().clear(); }

private:
    static StateList &states()
    {
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

static std::string option_preCheck_arg_type;
static std::string option_preCheck_arg_name;

static std::string option_tokens_namespace;
static std::string option_function_name = "classify";

std::stringstream input;
std::stringstream output;

std::string token_id(const std::string &id)
{
    std::string token = option_token_prefix;

    if (!option_toupper)
        token += id;
    else {
        for (size_t i = 0; i < id.size(); ++i)
            token += toupper(id[i]);
    }

    return token;
}

bool starts_with(const std::string &line, const std::string &text)
{
    if (text.length() <= line.length()) {
        return std::equal(line.begin(), line.begin() + text.size(), text.begin());
    }
    return false;
}

void doit(State &state)
{
    static int depth{0};
    static int preCheckDepth{0};

    ++depth;

    std::string indent(depth * 2, ' ');

    std::vector<char> firsts = state.firsts();
    for (std::vector<char>::iterator it = firsts.begin(); it != firsts.end(); ++it) {
        std::string _else = it == firsts.begin() ? "" : "else ";
        output << indent << _else << "if (";
        if (preCheckDepth == 0) {
            std::string commonPreCheck = state.commonPreCheck(*it);
            if (!commonPreCheck.empty()) {
                output << commonPreCheck << " && ";
                preCheckDepth++;
            }
        } else if (preCheckDepth > 0) {
            preCheckDepth++;
        }
        output << "s[" << (depth - 1) << "]" << option_unicode_function << " == '" << *it << "'";
        output << ") {" << std::endl;
        State &next_state = state.next(*it);

        bool found = false;
        for (DottedItemPtr item = next_state.first_item(); item != next_state.last_item(); ++item) {
            if (item->terminal()) {
                if (found) {
                    std::cerr << "*** Error. Too many accepting states" << std::endl;
                    exit(EXIT_FAILURE);
                }
                found = true;
                output << indent << "  return " << option_tokens_namespace
                       << token_id(item->rule->keyword) << ";" << std::endl;
            }
        }

        if (!found)
            doit(next_state);

        if (preCheckDepth > 0)
            preCheckDepth--;

        output << indent << "}" << std::endl;
    }

    --depth;
}

void gen_classify_n(State &start_state, size_t N)
{
    output << "static inline int " << option_function_name << N << "(const " << option_char_type
           << " *s";
    if (!option_preCheck_arg_type.empty()) {
        output << ", " << option_preCheck_arg_type;
        if (start_state.hasPreChecks()) {
            output << " " << option_preCheck_arg_name;
        }
    }
    output << ")" << std::endl << "{" << std::endl;
    doit(start_state);
    output << "  return " << option_tokens_namespace << token_id("identifier") << ";" << std::endl
           << "}" << std::endl
           << std::endl;
}

void gen_classify(const std::multimap<size_t, Rule> &keywords)
{
    output << "int " << option_namespace_name << option_function_name << "(const "
           << option_char_type << " *s, int n";
    if (!option_preCheck_arg_type.empty()) {
        output << ", " << option_preCheck_arg_type << " " << option_preCheck_arg_name;
    }
    output << ")" << std::endl;
    output << "{" << std::endl << "  switch (n) {" << std::endl;
    std::multimap<size_t, Rule>::const_iterator it = keywords.begin();
    while (it != keywords.end()) {
        size_t size = it->first;
        output << "    case " << size << ": return " << option_function_name << size << "(s";
        if (!option_preCheck_arg_type.empty()) {
            output << ", " << option_preCheck_arg_name;
        }
        output << ");" << std::endl;
        do {
            ++it;
        } while (it != keywords.end() && it->first == size);
    }
    output << "    default: return " << option_tokens_namespace << token_id("identifier") << ";"
           << std::endl
           << "  } // switch" << std::endl
           << "}" << std::endl
           << std::endl;
}

void gen_enums(const std::multimap<size_t, Rule> &keywords)
{
    output << "enum {" << std::endl;
    std::multimap<size_t, Rule>::const_iterator it = keywords.begin();
    for (; it != keywords.end(); ++it) {
        output << "  " << token_id(it->second.keyword) << "," << std::endl;
    }
    output << "  " << token_id("identifier") << std::endl << "};" << std::endl << std::endl;
}

inline bool not_whitespace_p(char ch)
{
    return !std::isspace(ch);
}

int main(int argc, char *argv[])
{
    const std::string ns = "--namespace=";
    const std::string inputFileOpt = "--input";
    const std::string outputFileOpt = "--output";

    std::string inputFilename;
    std::string outputFilename;

    for (int i = 0; i < argc; ++i) {
        const std::string arg(argv[i]);
        if (arg == "--no-enums")
            option_no_enums = true;
        else if (starts_with(arg, ns)) {
            option_namespace_name.assign(arg.begin() + ns.size(), arg.end());
            option_namespace_name += "::";
        } else if (arg == inputFileOpt && i + 1 < argc) {
            inputFilename = argv[i + 1];
            ++i;
        } else if (arg == outputFileOpt && i + 1 < argc) {
            outputFilename = argv[i + 1];
            ++i;
        }else if (arg == "--help" || arg == "-h") {
            std::cout << "usage: cplusplus-keywordgen [--input <kwgen file>] [--output <cpp file>]"
                      << std::endl;
            std::cout << "\t If no input or output specified: std::cin/cout will be used"
                      << std::endl;
            exit(EXIT_SUCCESS);
        }
    }

    if (inputFilename.empty()) {
        std::string textline;
        while (getline(std::cin, textline)) {
            input << textline << std::endl;
        }
    } else {
        std::ifstream fileInput(inputFilename, std::ios_base::in);
        std::string textline;
        while (getline(fileInput, textline)) {
            input << textline << std::endl;
        }
    }

    const std::string opt_no_enums = "%no-enums";
    const std::string opt_toupper = "%toupper";
    const std::string opt_ns = "%namespace=";
    const std::string opt_tok_prefix = "%token-prefix=";
    const std::string opt_char_type = "%char-type=";
    const std::string opt_unicode_function = "%unicode-function=";

    const std::string opt_preCheck_arg = "%pre-check-argument=";
    const std::string opt_function_name = "%function-name=";

    const std::string opt_no_namespace_for_tokens = "%no-namespace-for-tokens";

    // this may be only in keywords section
    const std::string preCheckOpt = "%pre-check=";

    bool useNamespaceForTokens = true;

    bool finished = false;
    while (!finished) {
        finished = true;

        bool readKeywords = false;
        std::string preCheckValue;

        std::multimap<size_t, Rule> keywords;
        std::string textline;

        while (getline(input, textline)) {
            // remove trailing spaces
            textline
                .assign(textline.begin(),
                        std::find_if(textline.rbegin(), textline.rend(), not_whitespace_p).base());

            if (!readKeywords) {
                if (textline.size() >= 2 && textline[0] == '%') {
                    if (textline[1] == '%') {
                        readKeywords = true;

                        static bool generatedMessageAdded=false;
                        if(!generatedMessageAdded){
                            generatedMessageAdded=true;
                            output
                                << "// === following code is generated with cplusplus-keywordgen tool"
                                << std::endl;
                            for (auto it = inputFilename.rbegin(); it != inputFilename.rend(); ++it) {
                                if (*it == '\\' || *it == '/') {
                                    output
                                        << "// === from source file: "
                                        << inputFilename.substr(std::distance(it, inputFilename.rend()))
                                        << std::endl;
                                    break;
                                }
                            }
                            output << std::endl;
                        }
                        output << "// === keywords begin" << std::endl;
                        output << std::endl;
                    } else if (textline == opt_no_enums) {
                        option_no_enums = true;
                    } else if (textline == opt_toupper) {
                        option_toupper = true;
                    } else if (starts_with(textline, opt_tok_prefix)) {
                        option_token_prefix.assign(textline.begin() + opt_tok_prefix.size(),
                                                   textline.end());
                    } else if (starts_with(textline, opt_char_type)) {
                        option_char_type.assign(textline.begin() + opt_char_type.size(),
                                                textline.end());
                    } else if (starts_with(textline, opt_unicode_function)) {
                        option_unicode_function.assign(textline.begin()
                                                           + opt_unicode_function.size(),
                                                       textline.end());
                    } else if (starts_with(textline, opt_ns)) {
                        option_namespace_name.assign(textline.begin() + opt_ns.size(),
                                                     textline.end());
                        option_namespace_name += "::";
                        if (useNamespaceForTokens) {
                            option_tokens_namespace = option_namespace_name;
                        }
                    } else if (starts_with(textline, opt_preCheck_arg)) {
                        std::string::size_type spacePos = textline.find(' ',
                                                                        opt_preCheck_arg.size());
                        if (spacePos == std::string::npos) {
                            option_preCheck_arg_type.clear();
                            option_preCheck_arg_name.clear();
                        } else {
                            option_preCheck_arg_type
                                = textline.substr(opt_preCheck_arg.size(),
                                                  spacePos - opt_preCheck_arg.size());
                            option_preCheck_arg_name = textline.substr(spacePos + 1);
                        }
                    } else if (starts_with(textline, opt_function_name)) {
                        option_function_name.assign(textline.begin() + opt_function_name.size(),
                                                    textline.end());
                    } else if (textline == opt_no_namespace_for_tokens) {
                        useNamespaceForTokens = false;
                        option_tokens_namespace.clear();
                    }

                    continue;
                }
                output << textline << std::endl;
            } else {
                if (textline.empty())
                    continue;

                if (textline == "%%") {
                    finished = false;
                    break;
                }

                if (starts_with(textline, preCheckOpt)) {
                    preCheckValue = textline.substr(preCheckOpt.size());
                }

                std::string::iterator start = textline.begin();
                while (start != textline.end() && std::isspace(*start))
                    ++start;

                std::string::iterator stop = start;
                while (stop != textline.end() && (std::isalnum(*stop) || *stop == '_'))
                    ++stop;

                if (start != stop) {
                    Rule rule;
                    rule.keyword.assign(start, stop);
                    if (rule.keyword == "identifier") {
                        std::cerr << "*** Error. `identifier' is reserved" << std::endl;
                        exit(EXIT_FAILURE);
                    }
                    rule.preCheck = preCheckValue;
                    keywords.insert(std::make_pair(rule.keyword.size(), rule));
                }
            }
        }

        if (readKeywords) {
            if (!option_no_enums)
                gen_enums(keywords);

            std::multimap<size_t, Rule>::iterator it = keywords.begin();
            while (it != keywords.end()) {
                size_t size = it->first;
                RuleList rules;
                do {
                    rules.push_back(it->second);
                    ++it;
                } while (it != keywords.end() && it->first == size);
                gen_classify_n(State::start(rules.begin(), rules.end()), size);
                State::reset();
            }

            gen_classify(keywords);

            output << "// === keywords end" << std::endl;
        }
    }

    if (outputFilename.empty()) {
        std::string textline;
        while (getline(output, textline)) {
            std::cout << textline << std::endl;
        }
    } else {
        std::ofstream outFile(outputFilename, std::ios_base::out);
        std::string textline;
        while (getline(output, textline)) {
            outFile << textline << std::endl;
        }
        std::cout << "Generated: " << outputFilename << std::endl;
    }
}
