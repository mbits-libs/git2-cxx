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


## Placeholders that expand to information extracted from the report

|Placeholder|Meaning|
|-|-|
|`%Hr`|report hash|
|`%hr`|abbreviated report hash|
|`%rP`|report parent hash|
|`%rp`|abbreviated report parent hash|
|`%rF`|report file list hash|
|`%rf`|abbreviated report file list hash|
|`%rD`|reported branch|
|`%rd`|report date (format respects --date= option)|
|`%rr`|report date, relative|
|`%rt`|report date, UNIX timestamp|
|`%ri`|report date, ISO 8601-like format|
|`%rI`|report date, strict ISO 8601 format|
|`%rs`|report date, short format (YYYY-MM-DD)|
|`%pP`|percentage|
|`%pT`|lines total|
|`%pR`|lines relevant|
|`%pC`|lines covered|
|`%pr`|lines rating, either `pass`, `incomplete` or `fail`, depending on how report coverage compares to preset ratings (currently 90% for passing and 75% for incomplete)|
|`%d`|ref names, like the --decorate option of git-log[1]|
|`%D`|ref names without the " (", ")" wrapping.|
|`%md`|ref names, each group colored separately, like the --decorate option of git-log[1]|
|`%mD`|ref names without the " (", ")" wrapping, each group colored separately.|

## Placeholders that expand to information extracted from the build

Within context of a build report, few report-oriented change meaning and refer to only the build and not whole report

|Placeholder|New meaning|
|-|-|
|`%Hr`|build hash|
|`%hr`|abbreviated build hash|
|`%pP`|build percentage|
|`%pT`|build lines total|
|`%pR`|build lines relevant|
|`%pC`|build lines covered|
|`%pr`|build lines rating, either `pass`, `incomplete` or `fail`, depending on how report coverage compares to preset ratings (currently 90% for passing and 75% for incomplete)|
|`%d`|build properties|
|`%D`|build properties without the " (", ")" wrapping.|
|`%md`|build properties, each property colored separately|
|`%mD`|build properties without the " (", ")" wrapping, each property colored separately.|

## Placeholders that expand to information extracted from the commit

Below, "person" is either author or committer.

|Placeholder|Meaning|
|-|-|
|`%Hc`|commit hash|
|`%hc`|abbreviated commit hash|
|`%an`, `%cn`|person name|
|`%ae`, `%ce`|person email|
|`%al`, `%cl`|person email local-part (the part before the @ sign)|
|`%ad`, `%cd`|person date (format respects --date= option)|
|`%ar`, `%cr`|person date, relative|
|`%at`, `%ct`|person date, UNIX timestamp|
|`%ai`, `%ci`|person date, ISO 8601-like format|
|`%aI`, `%cI`|person date, strict ISO 8601 format|
|`%as`, `%cs`|person date, short format (YYYY-MM-DD)|
|`%s`|subject|
|`%f`|sanitized subject line, suitable for a filename|
|`%b`|body|
|`%B`|raw body (unwrapped subject and body)|
