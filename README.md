# fastbin

Fast binary serialization for C++ and Julia data objects used for zero-copy IPC and efficient storage.

## Overview

The library is designed to be used in high-performance computing applications where serialization and deserialization speed is critical. The library does not perform any compression.

The data objects are defined in a JSON schema incl. enumeration types and docstrings.
A Python script generates the C++ and Julia code for the serialization and deserialization of the data objects. The generated code is then used in the C++ and Julia applications.

## Features

- Zero-copy serialization and deserialization
- Generation of C++ and Julia code from JSON schema file
- No dynamic memory allocation
- Documentation (docstrings) of data objects in JSON schema file
- Support for enumeration types
- Support for nested data objects
- Support for arrays of primitive types and fixed-size elements

### Supported types

| Schema type | C++ type         | Julia type       |
| ----------- | ---------------- | ---------------- |
| `int8`      | `int8_t`         | `Int8`           |
| `int16`     | `int16_t`        | `Int16`          |
| `int32`     | `int32_t`        | `Int32`          |
| `int64`     | `int64_t`        | `Int64`          |
| `uint8`     | `uint8_t`        | `UInt8`          |
| `uint16`    | `uint16_t`       | `UInt16`         |
| `uint32`    | `uint32_t`       | `UInt32`         |
| `uint64`    | `uint64_t`       | `UInt64`         |
| `float32`   | `float`          | `Float32`        |
| `float64`   | `double`         | `Float64`        |
| `char`      | `char`           | `UInt8`          |
| `byte`      | `std::byte`      | `UInt8`          |
| `string`    | `std::string`    | `StringView`     |
| `bool`      | `bool`           | `Bool`           |
| `vector<T>` | `std::vector<T>` | `Vector{T}`      |
| `enum`      | `enum class`     | `@enumx`         |
| `struct`    | `struct`         | `mutable struct` |

## Schema

The JSON schema file defines the data objects and enumeration types.
The schema file is used to generate the C++ and Julia code for the serialization and deserialization of the data objects.
JSON is preferred over other (prioprietary) schema formats because it is human-readable, easy to understand, and readable from a variety of programming languages, though not required for using the generated code.

```json
{
  "enums": {
    "MyEnum": {
      "type": "uint8",
      "docstring": "Documentation for MyEnum, also placed in generated code",
      "members": {
        "Unknown": { "value": 0 },
        "Second": { "value": 1, "docstring": "Member docstring." },
        "Third": { "value": 1, "docstring": "Member docstring of third member." },
        "Fourth": {
          "value": 1,
          "docstring": ["Member docstring of", "fourth member on multiple lines."]
        }
      }
    }
  },
  "structs": {
    "MyStruct": {
      "type": "struct",
      "members": [
        { "name": "a", "type": "int32" },
        { "name": "b", "type": "float64" },
        { "name": "c", "type": "string" },
        { "name": "d", "type": "vector<int32>" },
        { "name": "e", "type": "enum:MyEnum" }
      ]
    },
    "Child": {
      "members": [
        { "name": "a", "type": "int32" },
        { "name": "b", "type": "float64" }
      ]
    },
    "Parent": {
      "docstring": ["Docstring of Parent struct.", "Child needs to be defined before Parent in order to reference it."],
      "members": [
        { "name": "child", "type": "struct:Child" },
        { "name": "text", "type": "string" }
      ]
    }
  }
}
```

## Limitations

- No support for pointers or references
- No support for polymorphic types
- No support for arrays of variable-sized structs
- No support for unions
- No support for circular references
- Storage buffer needs to be allocated by the user manually with sufficient size
- Variable-sized members need to be set in order they appear in the schema

# Similar projects

- [FlatBuffers](https://google.github.io/flatbuffers/)
- [FastBinaryEncoding](https://github.com/chronoxor/FastBinaryEncoding)
- [Cap'n Proto](https://capnproto.org/)
- [Simple Binary Encoding](https://github.com/real-logic/simple-binary-encoding)
- [zpp::bits](https://github.com/eyalz800/zpp_bits)

# Internals

## Memory layout

The root object must be a schema-defined struct type.
Each member of the struct is serialized into a contiguous memory buffer in order they appear in the schema.
Nested data is serialized in a depth-first order.

The buffer is allocated by the user and needs to be large enough to hold the serialized object.

## 64-bit word size and alignment

Each member of a struct is aligned to 8 bytes (64-bit word size) in order to allow for efficient, direct access on 64-bit architectures.
Consequently, each member of a struct is padded to the next multiple of 8 bytes if necessary, possibly taking up more space than the member's native size (e.g. `bool`, `uint16`, etc.).
This is done to ensure that the struct as a whole is aligned to 8 bytes, and each member can be accessed directly, which is important for performance reasons.

## Fixed length members

Fixed length members are serialized directly into the buffer space without any additional information.
The size information is hardcoded into the generated code and is known at compile time.

## Variable length members

Variable length members additionally encode at the beginning of their buffer space the number of bytes they occupy using a `uint64` value.
If the variable length content possibly does not align to the 8 bytes word size, it is padded with zeros to the next multiple of 8 bytes.
The number of padding bytes is encoded in the `uint64` value at the beginning of the buffer space in the high-order 3 bits.

**Variable length size layout:**

     high bits                                                               low bits
    +-----------------+---------------------+----------------------------------------+
    | 5 reserved bits | 3 padding size bits |                      56 data size bits |
    +-----------------+---------------------+----------------------------------------+

The 56 bits equal roughly 72 petabytes of variable length data, which should be sufficient for nearly all use cases.
