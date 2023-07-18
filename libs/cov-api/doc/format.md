# The placeholders are

## Placeholders that expand to a single literal character

|Placeholder|Meaning|
|-|-|
|`%n`| newline |
|`%%`| a raw % |
|`%x00`| print a byte from a hex code |

## Placeholders that affect formatting of later placeholders

|Placeholder|Meaning|
|-|-|
|`%Cred`|switch color to red|
|`%Cgreen`|switch color to green|
|`%Cblue`|switch color to blue|
|`%Creset`|reset color|
|`%C(…​)`|color specification|
|`%w([<w>[,<i1>[,<i2>]]])`|switch line wrapping, like the -w option of git-shortlog[1].|

### Known colors

If there is an ✔ beside a color, it means this color could be prefixed with either `bold`, `bg`, `faint`, giving for example `%C(red)`, `%C(bold green)`, `%C(bg yellow)` or `%C(faint blue)`. Color named `rating` maps to `green` for passing, `yellow` for incomplete or `red` for failing rating, with prefixes preserved.

|color|allows prefixes|
|:--:|:--:|
|reset|✖|
|normal|✔|
|red|✔|
|green|✔|
|yellow|✔|
|blue|✔|
|magenta|✔|
|cyan|✔|
|rating|✔|

## Placeholders that expand to information extracted from various objects

### Object hashes

|Placeholder|Meaning|
|-|-|
|`%HR`, `%H1`|report hash or build hash|
|`%hR`, `%h1`|abbreviated report hash|
|`%HC`, `%H1`|contents hash (inside file object)|
|`%hC`, `%h1`|abbreviated contents hash (inside file object)|
|`%HF`, `%H2`|file list hash|
|`%hf`, `%h2`|abbreviated file list hash|
|`%HL`, `%H2`|line coverage hash (inside file object)|
|`%hL`, `%h2`|abbreviated line coverage hash (inside file object)|
|`%HP`, `%H3`|report parent hash|
|`%hP`, `%h3`|abbreviated report parent hash|
|`%Hf`, `%H3`|function coverage hash (inside file object)|
|`%hf`, `%h3`|abbreviated function coverage hash (inside file object)|
|`%HG`, `%H4`|commit hash|
|`%hG`, `%h4`|abbreviated commit hash|
|`%HB`, `%H4`|branch coverage hash (inside file object)|
|`%hB`, `%h4`|abbreviated branch coverage hash (inside file object)|

### Git commit information

Below, "person" is either author or committer. 

|Placeholder|Meaning|
|-|-|
|`%an`, `%cn`|person name|
|`%ae`, `%ce`|person email|
|`%al`, `%cl`|person email local-part (the part before the @ sign)|
|`%ad`, `%cd`|commit date (as if with strftime' `%c`)|
|`%ar`, `%cr`|commit date, relative|
|`%at`, `%ct`|commit date, UNIX timestamp|
|`%ai`, `%ci`|commit date, ISO 8601-like format|
|`%aI`, `%cI`|commit date, strict ISO 8601 format|
|`%as`, `%cs`|commit date, short format (YYYY-MM-DD)|
|`%s`|subject|
|`%f`|sanitized subject line, suitable for a filename|
|`%b`|body|
|`%B`|raw body (unwrapped subject and body)|

### Coverage summaries

|Placeholder|Meaning|
|-|-|
|`%pL`|code lines total|
|`%pPL`|lines percentage|
|`%pVL`|lines visited|
|`%pTL`|lines total|
|`%prL`|lines rating, either `pass`, `incomplete` or `fail`, depending on how report coverage compares to preset ratings (defaults 90% for passing and 75% for incomplete)|
|`%pPL`|functions percentage|
|`%pVL`|functions visited|
|`%pTL`|functions total|
|`%prL`|functions rating (defaults ?% for passing and ?% for incomplete)|
|`%pPB`|branches percentage|
|`%pVB`|branches visited|
|`%pTB`|branches total|
|`%prB`|branches rating (defaults ?% for passing and ?% for incomplete)|

### Report details

|Placeholder|Meaning|
|-|-|
|`%rD`|reported branch|
|`%rd`|report date (as if with strftime' `%c`)|
|`%rr`|report date, relative|
|`%rt`|report date, UNIX timestamp|
|`%ri`|report date, ISO 8601-like format|
|`%rI`|report date, strict ISO 8601 format|
|`%rs`|report date, short format (YYYY-MM-DD)|
|`%d`|ref names|
|`%D`|ref names without the " (", ")" wrapping|
|`%md`|ref names, each group colored separately|
|`%mD`|ref names without the " (", ")" wrapping, each group colored separately|


### Build details

In context of a build, not only `%HR` changes meaning to "hash of current build", but also these refer to that build, instead of the whole report:

|Placeholder|Meaning|
|-|-|
|`%rd`|build date (as if with strftime' `%c`)|
|`%rr`|build date, relative|
|`%rt`|build date, UNIX timestamp|
|`%ri`|build date, ISO 8601-like format|
|`%rI`|build date, strict ISO 8601 format|
|`%rs`|build date, short format (YYYY-MM-DD)|
|`%d`|build properties|
|`%D`|build properties without the " (", ")" wrapping|
|`%md`|build properties, each group colored separately|
|`%mD`|build properties without the " (", ")" wrapping, each group colored separately|

## Summary of information available in each context

|Placeholder|Type|REPORT|BUILD|FILES|FILE|
|-----------|----|------|-----|-----|----|
|PRIMARY|HASH|report (self)|build (self)||contents|
|SECONDARY|HASH|file list|file list|file list (self)|lines|
|TERTIARY|HASH|parent|||functions|
|QUATERNARY|HASH|commit|||branches|
|ADDED|TIME|added|added|||
|SUMMARY|COVERAGE|summary|summary||summary|
|REFs|LIST|refs|props|||
|BRANCH|TEXT|branch|||
|GIT INFO||git info|||

## Changing context

There is possibility of moving away from current context. This will change meaning of some of the placeholders, primarily hashes, but also ref placeholders become property placeholders, when context changes to a build.

Each context change starts with `%{<name>[` and ends with `%]}`. The `<name>` is a name of new context and may be empty. For example `%hR:%{B[ %hR%]}` in context of a report would print a short hash of the report, then  a space-separated short hash of each attached build.

```
7ea9628cf: 94b3a3180 e9274a33f
```

|Name|Context|Loops over|
|---|-------|----------|
|_empty_|_none_||
|`B`, `build`|report|builds|
|`F`, `files`|report, build, file list|files|

## Optional output

Optional output uses context change to loop over zero or one element, when the name starts with a question mark (`?`). For example `%{?prop[build %hR - %mD%n%]}` in context of a build would output anything only, if there are any properties attached to a build.

```
build c5d8aeaa6 - javac, debug=true, qnx
```

|Name|Context|Query|
|---|-------|----------|
|`?`, `?prop`|build|are there any properties attached to the build|
|`?pTL`|report, build, file|is total number of relevant lines non-zero|
|`?pTF`|report, build, file|is total number of reported functions non-zero|
|`?pTB`|report, build, file|is total number of reported branches non-zero|
