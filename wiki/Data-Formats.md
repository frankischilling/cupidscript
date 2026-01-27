# Data Formats

CupidScript provides built-in support for three widely-used data formats: CSV, YAML, and XML. These formats enable easy data interchange, configuration management, and structured data processing.

## Overview

| Format | Best For | Complexity | Human-Readable |
|--------|----------|------------|----------------|
| **CSV** | Tabular data, spreadsheets | Simple | Yes |
| **YAML** | Configuration files | Medium | Very |
| **XML** | Documents, structured data | Medium-High | Yes |
| **JSON** | APIs, general data | Simple | Yes |

## CSV (Comma-Separated Values)

CSV is ideal for simple tabular data and is compatible with spreadsheet applications.

### Quick Start

```cs
// Parse CSV
let csv_text = "name,age,city\nJohn,30,NYC\nAlice,25,LA";
let rows = csv_parse(csv_text);
print(rows);  // [["name", "age", "city"], ["John", "30", "NYC"], ...]

// Generate CSV
let data = [["name", "age"], ["Bob", "35"]];
let output = csv_stringify(data);
```

### Options

The `csv_parse()` function accepts an optional options map:

```cs
csv_parse(text, {
  delimiter: ",",      // Field separator (default: comma)
  quote: "\"",         // Quote character (default: double quote)
  headers: false,      // Treat first row as headers → list of maps
  skip_empty: false,   // Skip empty rows
  trim: false          // Trim whitespace from fields
})
```

### Examples

**Custom delimiter (tab-separated):**
```cs
let tsv = "a\tb\tc\n1\t2\t3";
let rows = csv_parse(tsv, {delimiter: "\t"});
```

**Headers mode:**
```cs
let csv = "name,age\nJohn,30\nAlice,25";
let records = csv_parse(csv, {headers: true});
// Returns: [{"name": "John", "age": "30"}, {"name": "Alice", "age": "25"}]
```

**Quoted fields:**
```cs
let csv = "name,description\nProduct,\"Contains, commas\"";
let rows = csv_parse(csv);  // Handles quoted fields correctly
```

## YAML (YAML Ain't Markup Language)

YAML is perfect for configuration files and human-readable data serialization.

### Quick Start

```cs
// Parse YAML
let yaml_text = "name: MyApp\nversion: 1.0.0\ndebug: true";
let config = yaml_parse(yaml_text);
print(config.name);     // "MyApp"
print(config.version);  // "1.0.0"
print(config.debug);    // true

// Generate YAML
let data = {"host": "localhost", "port": 8080};
let yaml = yaml_stringify(data);
```

### Data Types

YAML supports rich data types:

```yaml
# Strings
plain: hello
quoted: "hello world"

# Numbers
integer: 42
hex: 0xFF
octal: 0o755
float: 3.14

# Booleans
bool_true: true
bool_false: false
yes_val: yes
no_val: no

# Null
null_val: null

# Lists
items: [a, b, c]
# Or block style:
items:
  - a
  - b
  - c

# Maps
config: {debug: true, port: 8080}
# Or block style:
config:
  debug: true
  port: 8080
```

### Advanced Features

**Comments:**
```yaml
# This is a comment
name: MyApp  # Inline comment
```

**Multiline strings:**
```yaml
# Literal (preserves newlines)
literal: |
  Line 1
  Line 2

# Folded (joins lines)
folded: >
  This is a long
  line that folds
```

### Custom Indentation

```cs
let data = {"key": "value"};
let yaml2 = yaml_stringify(data, 2);  // 2-space indent
let yaml4 = yaml_stringify(data, 4);  // 4-space indent
```

## XML (Extensible Markup Language)

XML is ideal for documents, structured data, and data interchange.

### Quick Start

```cs
// Parse XML
let xml_text = "<person name=\"John\" age=\"30\"><city>NYC</city></person>";
let doc = xml_parse(xml_text);
print(doc.name);          // "person"
print(doc.attrs.name);    // "John"
print(doc.children[0].text);  // "NYC"

// Generate XML
let elem = {
  "name": "book",
  "attrs": {"isbn": "123"},
  "children": [
    {"name": "title", "text": "Guide"}
  ]
};
let xml = xml_stringify(elem);
```

### XML Structure

Parsed XML documents are represented as maps:

```cs
{
  "name": "element-name",
  "attrs": {               // Optional
    "attr1": "value1",
    "attr2": "value2"
  },
  "text": "text content",  // Optional (for simple text)
  "children": [            // Optional (for child elements)
    {"name": "child", ...}
  ]
}
```

### Examples

**Attributes:**
```cs
let xml = "<img src=\"photo.jpg\" alt=\"Photo\"/>";
let doc = xml_parse(xml);
print(doc.attrs.src);  // "photo.jpg"
print(doc.attrs.alt);  // "Photo"
```

**Nested elements:**
```cs
let xml = "<book><author>John</author><title>Guide</title></book>";
let doc = xml_parse(xml);
for child in doc.children {
  print(child.name, ":", child.text);
}
```

**Entity handling:**
```cs
let xml = "<text>Hello &lt;world&gt; &amp; friends</text>";
let doc = xml_parse(xml);
print(doc.text);  // "Hello <world> & friends"
```

**CDATA sections:**
```cs
let xml = "<script><![CDATA[if (x < 10) alert(\"low\");]]></script>";
let doc = xml_parse(xml);
print(doc.text);  // "if (x < 10) alert(\"low\");"
```

**Custom indentation:**
```cs
let elem = {"name": "root", "children": [...]};
let xml2 = xml_stringify(elem, 2);  // 2-space indent
let xml4 = xml_stringify(elem, 4);  // 4-space indent
let compact = xml_stringify(elem, 0);  // No formatting
```

## Common Patterns

### Configuration Files

**YAML config:**
```cs
let config_text = read_file("config.yaml");
let config = yaml_parse(config_text);

// Access nested config
print("Database:", config.database.host);
print("Port:", config.server.port);
```

**XML config:**
```cs
let config_text = read_file("config.xml");
let config = xml_parse(config_text);

// Navigate structure
for child in config.children {
  if (child.name == "database") {
    print("DB Host:", child.attrs.host);
  }
}
```

### Data Export

**Export to CSV:**
```cs
let data = [
  ["Name", "Score"],
  ["Alice", "95"],
  ["Bob", "87"]
];
let csv = csv_stringify(data);
write_file("scores.csv", csv);
```

**Export to XML:**
```cs
let report = {
  "name": "report",
  "children": [
    {"name": "date", "text": "2026-01-27"},
    {"name": "total", "text": "100"}
  ]
};
let xml = xml_stringify(report);
write_file("report.xml", xml);
```

### Data Conversion

**CSV to YAML:**
```cs
let csv = csv_parse(csv_text, {headers: true});
let yaml = yaml_stringify({"records": csv});
```

**XML to JSON:**
```cs
let xml_doc = xml_parse(xml_text);
let json = json_stringify(xml_doc);
```

## Performance Considerations

- **CSV**: Very fast for large tabular datasets
- **YAML**: Good for config files (< 1MB)
- **XML**: Good for structured documents (< 10MB)
- **JSON**: Fastest overall, use when possible

For very large files (> 10MB), consider:
- Processing in chunks
- Using streaming approaches
- Filtering data early

## Error Handling

All parsers return `nil` and set an error on parse failure:

```cs
let data = csv_parse(invalid_csv);
if (data == nil) {
  print("CSV parse failed");
}
```

Common errors:
- **CSV**: Unterminated quotes, inconsistent columns
- **YAML**: Invalid indentation, syntax errors
- **XML**: Unclosed tags, mismatched tags, invalid entities

## Comparison: When to Use Each Format

**Use CSV when:**
- Working with simple tabular data
- Need spreadsheet compatibility
- Data has uniform structure
- Performance is critical

**Use YAML when:**
- Writing configuration files
- Need human-readable format
- Have nested or complex data
- Comments are important

**Use XML when:**
- Need strict structure validation
- Working with document-oriented data
- Industry standards require XML (RSS, SVG, etc.)
- Need attributes and child elements together

**Use JSON when:**
- Building APIs
- Need fastest performance
- Wide language support required
- Data structure is simple

## See Also

- [Standard Library](Standard-Library.md) - Complete function reference
- [Strings and Strbuf](Strings-and-Strbuf.md) - String manipulation
- [Collections](Collections.md) - Working with lists and maps
