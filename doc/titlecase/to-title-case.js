/*
 * Derived from To Title Case 1.1.2 <http://individed.com/code/to-title-case/>
 * Copyright (c) 2008-2013 David Gouch, Fawzi Mohamed.
 * Licensed under the MIT License.
 */

String.prototype.toTitleCase = function() {
    return this.replace(/([\w&`'‘’"“.@:\/\{\(\[<>_]+-? *)/g, function(match, p1, index, title) {
        if (index > 0 && title.charAt(index - 2) !== ":" &&
            match.search(/^(a(mid|nd?|s|t)?|b(ut|y)|down|e(re|n)|f(or|rom)|i(f|n(to)?)|lest|next|o(n(to)?|ver|f|r)|per|re|t(h(en?|an|ru)|o|ill)|u(p|nto)|v(ia|s?\.?)|with)[ \-]/i) > -1)
// expanded list of the lowercase words:
//   a amid an and as at but by down ere en for from if in into lest
//   next on onto over of or per re then than thru to till up unto via
//   v vs v. vs.
            return match.toLowerCase();
        if (title.substring(index - 1, index + 1).search(/['"_{(\[]/) > -1)
            return match.charAt(0) + match.charAt(1).toUpperCase() + match.substr(2);
        if (match.substr(1).search(/[A-Z]+|&|[\w]+[._][\w]+/) > -1 ||
            title.substring(index - 1, index + 1).search(/[\])}]/) > -1)
            return match;
        return match.charAt(0).toUpperCase() + match.substr(1);
    });
};
