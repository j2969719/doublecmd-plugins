A subpattern can be named in one of three ways:
(?<name>...)
(?'name'...)
(?P<name>...)

Supporeted names
artist
title
album
track
year
genre

(?i) set caseless

Metacharacters outside square brackets
Character	Meaning

\	general escape character with several uses
^	assert start of string
$	assert end of string
.	match any character
[	start character class definition
|	start of alternative branch
(	start subpattern
)	end subpattern
?	extends the meaning of (, or 0/1 quantifier, or quantifier minimizer
*	0 or more quantifier
+	1 or more quantifier, also "possessive quantifier"
{	start min/max quantifier

Metacharacters inside square brackets
Character	Meaning
\	general escape character
^	negate the class, but only if the first character
-	indicates character range
[	POSIX character class
]	terminates the character class

Generic characters
Escape	Meaning

\d	any decimal digit
\D	any character that is not a decimal digit
\s	any whitespace character
\S	any character that is not a whitespace character
\w	any "word" character
\W	any "non-word" character

POSIX notation
Name	Meaning
[:alnum:]	letters and digits
[:alpha:]	letters
[:ascii:]	character codes 0 - 127
[:blank:]	space or tab only
[:cntrl:]	control characters
[:digit:]	decimal digits (same as \d)
[:graph:]	printing characters, excluding space
[:lower:]	lower case letters
[:print:]	printing characters, including space
[:punct:]	printing characters, excluding letters and digits
[:space:]	white space (not quite the same as \s)
[:upper:]	upper case letters
[:word:]	"word" characters (same as \w)
[:xdigit:]	hexadecimal digits

Unicode character properties
Escape	Meaning
\p{xx}	a character with the xx property
\P{xx}	a character without the xx property
\X	an extended Unicode sequence

The property names represented by xx above are limited to the Unicode script names,
the general category properties, and "Any", which matches any character (including newline).
Other properties such as "InMusicalSymbols" are not currently supported. Note that \P{Any}
does not match any characters, so always causes a match failure.


Properties
Ll	Letter, Lowercase
Lm	Letter, Modifier
Lo	Letter, Other
Lt 	Letter, TitlecaseLt
Lu	Letter, Uppercase
Mc	Mark, Spacing
Me	Mark, Enclosing
Mn	Mark, Nonspacing
Nd	Number, Decimal Digit
Nl	Number, Letter
No	Number, Other
Pc	Punctuation, Connector
Pd	Punctuation, Dash
Pe	Punctuation, Close
Pf	Punctuation, Final quote
Pi	Punctuation, Initial quote
Po	Punctuation, Other
Ps	Punctuation, Open
Sc	Symbol, Currency
Sk	Symbol, Modifier
Sm	Symbol, Math
So	Symbol, Other


The special property L& is also supported: it matches a character that has the Lu, Ll,
or Lt property, in other words, a letter that is not classified as a modifier or "other".

The long synonyms for these properties that Perl supports (such as \ep{Letter}) are not supported by
GRegex, nor is it permitted to prefix any of these properties with "Is".

Specifying caseless matching does not affect these escape sequences. For example, \p{Lu}
always matches only upper case letters.
The \X escape matches any number of Unicode characters that form an extended Unicode sequence.

Simple assertions
Escape	Meaning
\b	matches at a word boundary
\B	matches when not at a word boundary
\A	matches at the start of the string
\Z	matches at the end of the string or before a newline at the end of the string
\z	matches only at the end of the string
\G	matches at first matching position in the string

Lookahead assertions start with (?= for positive assertions and (?! for negative assertions.
Lookbehind assertions start with (?<= for positive assertions and (?<! for negative assertions.