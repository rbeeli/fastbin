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
- Support for arrays of fixed-size elements

### Supported languages

- C++20 (requires `std::span` and `std::byte`)
- Julia 1.6+ (requires packages `EnumX` and `StringViews`)

### Supported types

| Category    | Schema type | C++ type           | Julia type       |
| ----------- | ----------- | ------------------ | ---------------- |
| Primitive   | `int8`      | `int8_t`           | `Int8`           |
| Primitive   | `int16`     | `int16_t`          | `Int16`          |
| Primitive   | `int32`     | `int32_t`          | `Int32`          |
| Primitive   | `int64`     | `int64_t`          | `Int64`          |
| Primitive   | `uint8`     | `uint8_t`          | `UInt8`          |
| Primitive   | `uint16`    | `uint16_t`         | `UInt16`         |
| Primitive   | `uint32`    | `uint32_t`         | `UInt32`         |
| Primitive   | `uint64`    | `uint64_t`         | `UInt64`         |
| Primitive   | `float32`   | `float`            | `Float32`        |
| Primitive   | `float64`   | `double`           | `Float64`        |
| Primitive   | `char`      | `char`             | `UInt8`          |
| Primitive   | `byte`      | `std::byte`        | `UInt8`          |
| Primitive   | `bool`      | `bool`             | `Bool`           |
| Container   | `string`    | `std::string_view` | `StringView`     |
| Container   | `vector<T>` | `std::vector<T>`   | `Vector{T}`      |
| Enumeration | `enum`      | `enum class`       | `@enumx`         |
| Struct      | `struct`    | `struct`           | `mutable struct` |

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
      "docstring": [
        "Multi-line docstring of struct `Parent`.",
        "Note that `Child` needs to be defined in schema before `Parent` in order to reference it.",
        "",
        "Some more docstring."
      ],
      "members": [
        { "name": "child", "type": "struct:Child" },
        { "name": "text", "type": "string" }
      ]
    }
  }
}
```

## Limitations

- Storage buffer needs to be allocated by the user manually with sufficient size
- Variable-sized members need to be set in order they appear in the schema
- No support for pointers or references
- No support for polymorphic types
- No support for arrays of variable-sized elements (e.g. `vector<string>`, `vector<struct:MyVarStruct>`)
- No support for unions
- No support for circular references

# Similar projects

- [FlatBuffers](https://google.github.io/flatbuffers/)
- [FastBinaryEncoding](https://github.com/chronoxor/FastBinaryEncoding)
- [Cap'n Proto](https://capnproto.org/)
- [Simple Binary Encoding](https://github.com/real-logic/simple-binary-encoding)
- [zpp::bits](https://github.com/eyalz800/zpp_bits)

# Internals

## Memory layout

The object tree, as defined in the JSON schema, is serialized into a contiguous memory buffer.
The root object must be a schema-defined struct type.
Each member of the struct is serialized into a contiguous memory buffer in the order they appear in the schema.

Consequently, the order in which variable-sized members are set MUST match the order in the schema definition, e.g. for strings, vectors or variable-sized struct members.

The storage buffer needs to be allocated by the user and must to be large enough to hold the entire serialized struct and all its members.

## 64-bit word size and alignment

Each member of a struct is aligned to 8 bytes (64 bit word size) in order to allow for efficient, direct access on 64-bit architectures.
Consequently, each member of a struct is padded to the next multiple of 8 bytes if necessary, possibly taking up more space than the member's native size (e.g. `bool`, `uint16`, etc.).
This is done to ensure that the struct as a whole is aligned to 8 bytes as well, and each member can be accessed directly.

Aligning to 8-byte boundaries can enhance performance by allowing the CPU to access memory more efficiently, reducing the likelihood of cache misses.
On ARM architectures, the alignment of primitive types must be respected according to their size, or a multiple of their size, otherwise the CPU will throw an alignment fault.
On x86 architectures, the alignment of primitive types is not strictly required, but it can still improve performance.

## Fixed length members

Fixed length members are serialized directly into the buffer space without any additional metadata.
Fixed size member have their size hardcoded into the generated code and is known at compile time, thus does not need to be written into the buffer.

## Variable length members

Variable length members additionally encode the number of bytes they occupy using a `uint64` value at the beginning of their buffer space.
This number includes the number of bytes the `uint64` value occupies, not just number of bytes of the member's data.

If the member's data does not align to 8 bytes, padding is added to reach the next multiple of 8 bytes.
The number of added padding bytes (0-7) is encoded in the 8 high-order bits of the `uint64` size value at the beginning of the buffer space.

**Variable length size metadata layout:**

    high bits                                                                       low bits
    +----------------------+-----------------+---------------------------------------------+
    | 5 reserved bits      | 3 pad size bits |                         56 member size bits |
    +----------------------+-----------------+---------------------------------------------+

The 56 bits for encoding the member size equal roughly 72 petabytes, which should be sufficient for all use cases.

It is advisable, though not required, to define the variable-sized members at the end of the struct members in the schema.
This allows to hardcode the buffer offsets of all fixed-size members in the generated code, since they do not depend on the size of preceding variable-sized members.
