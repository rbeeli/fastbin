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
| `string`    | `std::string`    | `StringView`     |
| `bool`      | `bool`           | `Bool`           |
| `vector<T>` | `std::vector<T>` | `Vector{T}`      |
| `enum`      | `enum class`     | `@enumx`         |
| `struct`    | `struct`         | `mutable struct` |

## Limitations

- No support for pointers or references
- No support for polymorphic types
- No support for arrays of variable-sized structs
- No support for unions
- No support for circular references
- Storage buffer needs to be allocated by the user manually with sufficient size
- Variable-sized members need to be set in order they appear in the schema
