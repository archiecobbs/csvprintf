**csvprintf** is a simple UNIX command line utility for parsing CSV files.

**cvsprintf** works just like the `printf(1)` command line utility. You supply a `printf(1)` format string on the command line and each record in the CSV file is formatted accordingly. Each format specifier in the format string contains a column accessor to specify which CSV column to use, so for example `%3$d` would format the third column as a decimal value.

**csvprintf** can also convert CSV files into XML documents.

You can view the [ManPage](https://github.com/archiecobbs/csvprintf/wiki/ManPage) online.

### Examples

Given this input file `input.csv`:

```
NAME,ADDRESS,POINTS
Fred Smith,"1234 Main St.
Anytown, USA   39103",123.4567
"Wayne ""The Great One"" Gretsky",  59 Hockey Lane  , 999999
```

here is the resulting output:

```
$ cat input.csv | csvprintf -i 'Name:    [%1$-24.24s]\nAddress: [%2$-12.12s]\nPoints:  %3$.2f\n'
Name:    [Fred Smith              ]
Address: [1234 Main St]
Points:  123.46
Name:    [Wayne "The Great One" Gr]
Address: [59 Hockey La]
Points:  999999.00
```

An example of the XML output:

```
$ cat input.csv | csvprintf -ix
<?xml version="1.0" encoding="ISO-8859-1"?>
<csv>
  <row>
    <col1>Fred Smith</col1>
    <col2>1234 Main St.
Anytown, USA   39103</col2>
    <col3>123.4567</col3>
  </row>
  <row>
    <col1>Wayne &quot;The Great One&quot; Gretsky</col1>
    <col2>59 Hockey Lane</col2>
    <col3>999999</col3>
  </row>
</csv>
```
