# Objects internal structure

Each structure consists of 32 unsigned int numbers. They are stored and loaded in memory order, which means they follow current architecture endiannes. In tables below, each offset and size is in 4-byte increments (so offset 10 means 40 bytes from beginning of the file).

After un-gzipping, each file start with a file header, consisting of magic and version.

## Types

|Type|Description|
|----|-----------|
|**uint**|32-bit integer without a sign. E.g. `uint32_t`.|
|**magic**|**uint** with recognizable mnemonic.|
|**version**|**uint** with major in upper 16 bits and minor in lower 16 bits. Currently, only version 1.0 (or `0x0001'00000`) is defined, but changes to minor **must** be backwards compatible.|
|**str**|**uint** referencing into file's string **block**|
|**oid**|20 byte (4 **uint**) SHA256 reference to another object.|
|**timestamp**|**uint64** number of seconds since UNIX epoch; when looking at it as a pair of **uint**s, first will contain upper 32bits and second - lower 32 bits of timestamp.|
|**UTF8Z**|Block of ASCIIZ-like strings. Like ASCIIZ, each string ends with a byte zero, unlike ASCIIZ, each string is encoded with UTF-8.|

### Common structures

#### block

|Offset|Size|Value|Ref|Type|
|-----:|---:|-----|---|----|
|0|1|offset|`SO`|uint|
|1|1|size|`SIZE`|uint|

#### array_ref

|Offset|Size|Value|Ref|Type|
|-----:|---:|-----|---|----|
|0|1|offset|`EO`|uint|
|2|1|size|`ES`|uint|
|3|1|count|`EC`|uint|

#### stats

|Offset|Size|Value|Type|
|-----:|---:|-----|----|
|0|1|total|uint|
|1|1|visited|uint|

## REPORT

|Offset|Size|Value|Type|
|-----:|---:|-----|----|
||||**_file header_**|
|0|1|`"rset"`|magic|
|1|1|1.0|version|
||||**_header_**|
|2|2|strings|block|
|4|5|parent|oid|
|4|5|file_list|oid|
|9|3|builds|array_ref|
|12|2|added|timestamp|
|||git|**_report::commit_**|
|14|1|branch|str|
|15|2|author|email_ref|
|17|2|committer|email_ref|
|19|1|message|str|
|20|5|commit_id|oid|
|25|2|committed|timestamp|
|||stats|**_coverage_stats_**
|27|1|lines_total|uint|
|28|2|lines|stats|
|30|2|functions|stats|
|32|2|branches|stats|
|`SO`|`SIZE`|bytes|UTF8Z|
|`EO`|`ES`&times;`EC`|entries|report_set_entry[`EC`]|

### report::build

|Offset|Size|Value|Type|
|-----:|---:|-----|----|
|0|5|build|oid|
|5|1|propset|str|
|||set|**_coverage_stats_**
|6|1|lines_total|uint|
|7|2|lines|stats|
|9|2|functions|stats|
|11|2|branches|stats|

### email_ref

|Offset|Size|Value|Type|
|-----:|---:|-----|----|
|0|1|name|str|
|1|1|email|str|


## BUILD

|Offset|Size|Value|Type|
|-----:|---:|-----|----|
||||**_file header_**|
|0|1|`"rprt"`|magic|
|1|1|1.0|version|
||||**_report_header_**|
|2|1|strings|block|
|3|5|file list|oid|
|8|2|added|timestamp|
|10|1|propset|str|
||||**_coverage_stats_**|
|11|1|lines_total|uint|
|12|2|lines|stats|
|14|2|functions|stats|
|16|2|branches|stats|
||||**_strings_**|
|`SO`|`SIZE`|bytes|UTF8Z|


## FILES

|Offset|Size|Value|Type|
|-----:|---:|-----|----|
||||**_file header_**|
|0|1|`"list"`|magic|
|1|1|1.0|version|
||||**_files_**|
|2|2|strings|block|
|4|3|entries|array_ref|
|`SO`|`SIZE`|bytes|UTF8Z|
|`EO`|`ES`&times;`EC`|entries|files::entry[`EC`]|

### report_entry_stats

|Offset|Size|Value|Type|
|-----:|---:|-----|----|
|0|2|summary|stats|
|2|5|details|oid|


### files::entry

Report entry has variadic length. For reports, where no file has reported function or branch coverage, the `EC` will be 14 **uint**s. If at least one file has function or branch coverage, `EC` will be 28 **uint**s.

#### `EC` == 14, files::basic

|Offset|Size|Value|Type|
|-----:|---:|-----|----|
|0|1|path|str|
|1|5|contents|oid|
|6|1|lines_total|uint|
|7|7|lines|report_entry_stats|

#### `EC` > 14, files::ext

|Offset|Size|Value|Type|
|-----:|---:|-----|----|
|14|7|functions|report_entry_stats|
|21|7|branches|report_entry_stats|

## LINE COVERAGE

|Offset|Size|Value|Ref|Type|
|-----:|---:|-----|---|----|
|||||**_file header_**|
|0|1|`"lnes"`||magic|
|1|1|1.0||version|
|||||**_line_coverage_**|
|2|1|line_count|`LC`|uint|
|3|`LC`|coverage||uint|

### coverage

To get the line number, start with 1. Then, if `flag` is set, add `value` to the line number, otherwise increment line number by 1. If `flag` is cleared, `value` is execution count for current line.

|Bit from|Bit to|Value|Type|
|-----:|---:|-----|----|
|31|31|is_null|flag|
|30|0|value|uint|

## FUNCTION COVERAGE

|Offset|Size|Value|Type|
|-----:|---:|-----|----|
||||**_file header_**|
|0|1|`"fnct"`|magic|
|1|1|1.0|version|
||||**_function_coverage_**|
|2|2|strings|block|
|4|3|entries|array_ref|
|`SO`|`SIZE`|bytes|UTF8Z|
|`EO`|`ES`&times;`EC`|entries|function_coverage_entry[`EC`]|

### function_coverage_entry

For each line or column number, 0 denotes that value is missing. Also, a missing demangled name would point to a string of length 0.

|Offset|Size|Value|Type|
|-----:|---:|-----|----|
|0|1|name|str|
|1|1|demangled_name|str|
|2|1|count|uint|
|3|1|line_start|uint|
|4|1|column_start|uint|
|5|1|line_end|uint|
|6|1|column_end|uint|
