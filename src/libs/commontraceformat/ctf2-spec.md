# CTF2‑SPEC‑2.0rB: Common Trace Format version 2


Philippe Proulx
[pproulx@efficios.com](mailto:pproulx@efficios.com)
version 2.0rB,
22 May 2026


Table of Contents

[1. Revision history](#_revision_history)

- [2. What’s CTF 2?](#_whats_ctf2)

- [3. Common definitions](#_common_definitions)

- [4. Trace composition](#trace)

[4.1. Metadata stream (overview)](#metadata-stream-overview)

- [4.2. Data stream](#ds)

[4.2.1. Packet](#pkt)

- [4.2.2. Event record](#er)


- [5. Metadata stream](#metadata-stream)

[5.1. Extensions](#ext)

- [5.2. Attributes](#attrs)

- [5.3. Field classes](#fc)

[5.3.1. Field location](#field-loc)

- [5.3.2. Integer range set](#int-range-set)

- [5.3.3. Roles](#roles)

- [5.3.4. Fixed-length bit array field class](#fl-ba-fc)

- [5.3.5. Fixed-length bit map field class](#fl-bm-fc)

- [5.3.6. Fixed-length boolean field class](#fl-bool-fc)

- [5.3.7. Abstract integer field class](#int-fc)

- [5.3.8. Fixed-length integer field class](#fl-int-fc)

- [5.3.9. Fixed-length floating point number field class](#fl-fp-fc)

- [5.3.10. Variable-length integer field class](#vl-int-fc)

- [5.3.11. Abstract string field class](#str-fc)

- [5.3.12. Null-terminated string field class](#nt-str-fc)

- [5.3.13. Static-length string field class](#sl-str-fc)

- [5.3.14. Dynamic-length string field class](#dl-str-fc)

- [5.3.15. Abstract BLOB field class](#blob-fc)

- [5.3.16. Static-length BLOB field class](#sl-blob-fc)

- [5.3.17. Dynamic-length BLOB field class](#dl-blob-fc)

- [5.3.18. Structure field class](#struct-fc)

- [5.3.19. Abstract array field class](#array-fc)

- [5.3.20. Static-length array field class](#sl-array-fc)

- [5.3.21. Dynamic-length array field class](#dl-array-fc)

- [5.3.22. Optional field class](#opt-fc)

- [5.3.23. Variant field class](#var-fc)


- [5.4. Preamble fragment](#preamble-frag)

- [5.5. Field class alias fragment](#fca-frag)

- [5.6. Trace class fragment](#tc-frag)

[5.6.1. Roles](#_roles)


- [5.7. Clock class fragment](#cc-frag)

[5.7.1. Clock origin](#clock-origin)

- [5.7.2. Clock offset](#clock-offset)


- [5.8. Data stream class fragment](#dsc-frag)

[5.8.1. Roles](#_roles_2)


- [5.9. Event record class fragment](#erc-frag)


- [6. Data stream decoding procedure](#ds-dec)

[6.1. Packet decoding procedure](#pkt-dec)

- [6.2. Event record decoding procedure](#er-dec)

- [6.3. Clock value update procedure](#clk-val-update)

- [6.4. Field decoding procedure](#field-dec)

[6.4.1. Alignment procedure](#align-dec)

- [6.4.2. Field location procedure](#field-loc-dec)

- [6.4.3. Fixed-length bit array field decoding procedure](#fl-ba-field-dec)

- [6.4.4. Fixed-length bit map field decoding procedure](#fl-bm-field-dec)

- [6.4.5. Fixed-length boolean field decoding procedure](#fl-bool-field-dec)

- [6.4.6. Fixed-length unsigned integer field decoding procedure](#fl-uint-field-dec)

- [6.4.7. Fixed-length signed integer field decoding procedure](#fl-sint-field-dec)

- [6.4.8. Fixed-length floating point number field decoding procedure](#fl-fp-field-dec)

- [6.4.9. Variable-length unsigned integer field decoding procedure](#vl-uint-field-dec)

- [6.4.10. Variable-length signed integer field decoding procedure](#vl-sint-field-dec)

- [6.4.11. Null-terminated string field decoding procedure](#nt-str-field-dec)

- [6.4.12. Static-length string field decoding procedure](#sl-str-field-dec)

- [6.4.13. Static-length BLOB field decoding procedure](#sl-blob-field-dec)

- [6.4.14. Dynamic-length string field decoding procedure](#dl-str-field-dec)

- [6.4.15. Dynamic-length BLOB field decoding procedure](#dl-blob-field-dec)

- [6.4.16. Structure field decoding procedure](#struct-field-dec)

- [6.4.17. Static-length array field decoding procedure](#sl-array-field-dec)

- [6.4.18. Dynamic-length array field decoding procedure](#dl-array-field-dec)

- [6.4.19. Optional field decoding procedure](#opt-field-dec)

- [6.4.20. Variant field decoding procedure](#var-field-dec)


This document is the official specification of the Common Trace
Format (CTF) version 2 (***CTF2‑SPEC‑2.0***),
as published by the [DiaMon Workgroup](https://diamon.org/).


| ** | RFC 2119 The key words *MUST*, *MUST NOT*, *REQUIRED*, *SHOULD*, *SHOULD NOT*, *MAY*, and *OPTIONAL* in this document, when emphasized, are to be interpreted as described in [RFC 2119](https://datatracker.ietf.org/doc/html/rfc2119). |
|---|---|


| ** | Document identification The name of this document (***CTF2‑SPEC‑2.0rB***) follows the specification of [CTF2‑DOCID‑2.0](https://diamon.org/ctf/CTF2-DOCID-2.0.html). |
|---|---|


## 1. Revision history


| Document | Publication date | Changes |
|---|---|---|
| ***CTF2‑SPEC‑2.0rB*** | 22 May 2026 | In the field [decoding procedures](#ds-dec), use “>” instead of “≥” when comparing a decoding position to the packet content length (***PKT_CONTENT_LEN***) so that a field may end exactly at the packet content boundary. Fix the [variable-length unsigned integer](#vl-uint-field-dec) and [variable-length signed integer](#vl-sint-field-dec) field decoding procedures so that they append the first decoded byte to the resulting byte sequence. In the [optional](#opt-field-dec) and [variant](#var-field-dec) field decoding procedures, clarify the handling of boolean vs. integer selector field values. In the [variant](#var-field-dec) field decoding procedure, report an error when no variant field class option matches the selector field value. For the [trace class](#tc-frag), the [clock class](#cc-frag), the [data stream class](#dsc-frag), and the [event record class](#erc-frag) fragments, clarify that the `namespace`, `name`, and `uid` properties are those of the class, not of an instance. The corresponding ***CTF2‑SPEC‑2.0rA*** revision was meant to address this, but, in error, those property descriptions were left unchanged. Use “␞” and “…” symbols in examples instead of `<RS>` and `[...]`. Fix various typos and other minor errors throughout. |
| ***CTF2‑SPEC‑2.0rA*** | 14 November 2024 | For the [clock class fragment](#cc-frag), the [data stream class fragment](#dsc-frag), and the [event record class fragment](#erc-frag), fix the following replication errors: The `namespace` property is the [namespace](#ns-def) of the class, not of an instance. The `name` property is the name of the class, not of an instance. The `uid` property is the unique ID of the class, not of an instance. |
| ***CTF2‑SPEC‑2.0*** | 26 March 2024 | Initial specification of CTF 2. |


The latest CTF 1 version is, as of 22 May 2026,
[CTF 1.8.3](https://diamon.org/ctf/v1.8.3/).


## 2. What’s CTF 2?


The ***Common Trace Format*** version 2 is a binary
[trace](https://en.wikipedia.org/wiki/Tracing_(software)) format designed
to be very fast to write without compromising great flexibility.


The intention of CTF 2 is that applications written in any programming
language, and running on any system (be it Linux or bare metal, for
example), can generate traces natively.


A CTF 2 trace has all its [data streams](#ds) described by a
[metadata stream](#metadata-stream-overview). Given the rich set of
supported [data field types](#fc), this makes it possible for a CTF 2
[producer](#producer-def) to append binary data structures as is to data
streams without further data transformation. Indeed, the length,
alignment, byte order, and bit order of [fixed-length bit array fields](#fl-ba-fc)
are all configurable parameters within the metadata stream.


CTF 2 is transport-agnostic: this document doesn’t specify how to
transport or store CTF 2 streams. Other documents can specify such
conventions, and conform CTF 2 producers and [consumers](#consumer-def)
may or may not adhere to them.


CTF 2 is a major revision of [CTF 1](https://diamon.org/ctf/v1.8.3/), bringing many improvements, such
as:


- Using JSON text sequences for the metadata stream.

- Simplifying the metadata stream.

For example, [field class aliases](#fca-frag) may only exist at the
root level.

- Adding new [field classes](#fc) and improving existing ones.

- Supporting the [UTF-16](https://en.wikipedia.org/wiki/UTF-16) and
[UTF-32](https://en.wikipedia.org/wiki/UTF-32) encodings of
[string fields](#str-fc).

- Using [roles](#roles) instead of reserved structure member names to
identify meaningful fields.

- Adding the [attribute](#attrs) and [extension](#ext) features to
extend and customize the format.


and more, while remaining backward compatible at the data stream level.


## 3. Common definitions


Common CTF 2 definitions:


 [Byte](#byte-def)

A group of eight [bits](https://en.wikipedia.org/wiki/Bit) operated on
as a unit.


The bits are indexed such that, if the byte represents an 8-bit unsigned
integer, bit 0 is the
[least
significant](https://en.wikipedia.org/wiki/Bit_numbering#Least_significant_bit) and bit 7 is the
[most
significant](https://en.wikipedia.org/wiki/Bit_numbering#Most_significant_bit).


In the diagrams of this document, when a byte is represented as eight
contiguous squares, the rightmost one is always bit 0:


*


 [Class](#class-def)

A set of values (instances) which share common properties.


For example, a [fixed-length unsigned integer field class](#fl-int-fc) with an 8-bit length property is the set of
all the fixed-length unsigned integer fields from binary `00000000` to `11111111`
(integers 0 to 255).


This specification often states that some class *describes* instances.
For example, an [event record class](#erc-frag) describes [event
records](#er).


 [Consumer](#consumer-def)

A software or hardware system which consumes (reads) the streams of
a [trace](#trace).


A trace consumer is often a *trace viewer* or a *trace analyzer*.


 [Namespace](#ns-def)

A string of which the purpose is to avoid naming conflicts.


This document doesn’t specify the format of a namespace. CTF 2
recommends to at least include a domain name owned by the organization
defining the objects under a namespace as well as its registration year,
for example `example.com,1995`.


| * | The `diamon.org,2012` namespace and its `std` alias are reserved for official [DiaMon Workgroup](https://diamon.org/) CTF 2 documents. |
|---|---|


 [Producer](#producer-def)

A software or hardware system which produces (writes) the streams of
a [trace](#trace).


A trace producer is often a *tracer*.


 [Sequence](#seq-def)

A set of related items which follow each other in a particular
order.


 [Stream](#stream-def)

A [sequence](#seq-def) of [bytes](#byte-def).


## 4. Trace composition


A trace is:


- One [metadata stream](#metadata-stream-overview).

- One or more [data streams](#data-stream).


As a reminder, this specification defines a [stream](#stream-def) as a
sequence of bytes.


| ** | This document doesn’t specify how to transport or store CTF 2 streams. A [producer](#producer-def) could serialize all streams as a single file on the file system, or it could send the streams over the network using TCP, to name a few examples. |
|---|---|


### 4.1. Metadata stream (overview)


A metadata stream describes trace [data streams](#ds) with JSON objects.


A metadata stream describes things such as:


- The [class](#cc-frag) of the data stream [default clocks](#def-clk).

- The names of [event record classes](#erc-frag).

- The [classes](#fc) of event record fields.


Multiple traces *MAY* share the same metadata stream.


See [Metadata stream](#metadata-stream) for the full metadata stream specification.


### 4.2. Data stream


A *data stream* is a [sequence](#seq-def) of one or more data
[packets](#pkt):


*


In the [metadata stream](#metadata-stream), a
[data stream class](#dsc-frag) describes data streams.


A packet *MUST* contain one or more bytes of data.


Although a packet *MAY* contain padding (garbage data) at the end
itself, from the point of view of a data stream, there’s no padding
between packets. In other words, the byte following the last byte of a
packet is the first byte of the next packet.


A data stream *MAY* have, conceptually:


 One default, monotonic clock

Described by a [clock class](#cc-frag) in the metadata stream.


[Packets](#pkt) and [event records](#er) *MAY* contain snapshots, named
*timestamps*, of the default clock of their data stream.


 One counter of discarded event records

Indicates the number of event records which the
[producer](#producer-def) needed to discard for different reasons.


For example, a tracer could discard an event record when it doesn’t fit
some buffer and there’s no other available buffer.


A packet *MAY* contain a snapshot of this counter.


See [Data stream decoding procedure](#ds-dec) to learn how to decode a CTF 2 data stream.


#### 4.2.1. Packet


A *packet* is a segment of a [data stream](#ds).


A packet contains a [sequence](#seq-def) of data *fields* or padding
(garbage data). In the metadata stream, [field classes](#fc) describe
data fields.


A packet ***P***, contained in a data stream ***S***, contains,
in this order:


- *OPTIONAL*: A **header** [structure](#struct-fc) field,
described at the [trace class](#tc-frag) level in the
[metadata stream](#metadata-stream), which contains, in this order:


*OPTIONAL*: A [fixed-length unsigned integer](#fl-int-fc) field which encodes the
packet magic number value (0xc1fc1fc1, or 3,254,525,889).

- In any order:


*OPTIONAL*: One or more [static-length BLOB](#sl-blob-fc) fields which
encode the [metadata stream UUID](#metadata-stream-uuid).

- *OPTIONAL*: One or more unsigned integer fields which encode the
current numeric ID of the [class](#dsc-frag) of ***S***.

- *OPTIONAL*: One or more unsigned integer fields which encode the
current numeric ID of ***S***.


- *OPTIONAL*: A **context** [structure](#struct-fc) field,
described at the [data stream class](#dsc-frag) level in the metadata
stream, which contains, in any order:


*OPTIONAL*: One or more unsigned integer fields which encode the
current total length of ***P***, in bits (always a multiple
of 8).

- *OPTIONAL*: One or more unsigned integer fields which encode the
current content length of ***P***, in bits.

- *OPTIONAL*: One or more unsigned integer fields which encode the
current beginning timestamp or partial timestamp of ***P***.

- *OPTIONAL*: One or more unsigned integer fields which encode the
current end timestamp of ***P***.

- *OPTIONAL*: One or more unsigned integer fields which encode the
current snapshot of the [discarded event record
counter](#disc-er-counter) of ***S*** at the end of ***P***.

- *OPTIONAL*: One or more unsigned integer fields which encode the
current sequence number of ***P*** within ***S***.

- *OPTIONAL*: User fields.


- Zero or more [event records](#er).


A packet *MUST* contain one or more bytes of data.


A packet *MAY* contain padding (garbage data) after its *last* event
record. The length of this padding is the difference between its total
length and its content length (as found in its [context
structure field](#pkt-ctx)).


Packets are independent of each other: if one removes a packet from a
data stream, a [consumer](#consumer-def) can still decode the whole data
stream. This is why:


- Packets *MAY* contain *snapshots* of the [discarded
event record counter](#disc-er-counter) of their data stream.

- Packets and event records *MAY* contain *timestamps* which are
snapshots of the [default clock](#def-clk) of their data stream.


If the [packet context](#pkt-ctx) fields of the packets of a data stream
contain a [packet sequence number](#pkt-seq-num-role) field, a consumer
can recognize missing packets.


See [Packet decoding procedure](#pkt-dec) to learn how to decode a CTF 2 packet.


#### 4.2.2. Event record


An *event record* is the result of a [producer](#producer-def) writing a
record with *OPTIONAL* user data when an event occurs during its
execution.


A [packet](#pkt) contains zero or more event records.


An [event record class](#erc-frag) describes the specific parts of event
records.


An event record ***E***, contained in a [data stream](#ds) ***S***,
contains, in this order:


- *OPTIONAL*: A **header** [structure](#struct-fc) field,
described at the [data stream class](#dsc-frag) level in the metadata
stream, which contains, in any order:


*OPTIONAL*: One or more unsigned integer fields which encode the
numeric ID of the [class](#erc-frag) of ***E*** which has the
class of ***S*** as its parent.

- *OPTIONAL*: One or more unsigned integer fields which encode a
timestamp or a partial timestamp.


- *OPTIONAL*: A **common context**
[structure](#struct-fc) field, described at the data stream class
level in the metadata stream, which contains user fields.

- *OPTIONAL*: A **specific context**
[structure](#struct-fc) field, described at the event record class
level in the metadata stream, which contains user fields.

- *OPTIONAL*: A **payload** [structure](#struct-fc) field,
described at the event record class level in the metadata stream,
which contains user fields.


An event record *MUST* contain one or more bits of data.


The [default clock](#def-clk) timestamp of an event
record ***E***, that is, the value of the default clock of its
[data stream](#ds) *after* the [header field](#er-header) ***F***
of ***E*** (if ***F*** exists) is encoded/decoded,
*MUST* be greater than or equal to the default clock timestamp
of the previous event record, if any, within the *same* data stream.


See [Event record decoding procedure](#er-dec) to learn how to decode a CTF 2 event record.


## 5. Metadata stream


A metadata stream is a JSON text sequence, as specified by [RFC 7464](https://datatracker.ietf.org/doc/html/rfc7464),
of *fragments*:


> In prose: any number of JSON texts, each encoded in UTF-8
> [[RFC3629](https://datatracker.ietf.org/doc/html/rfc3629)], each
> preceded by one ASCII RS character, and each followed by a line
> feed (LF).
> Since RS is an ASCII control character, it may only appear in JSON
> strings in escaped form (see
> [[RFC7159](https://datatracker.ietf.org/doc/html/rfc7159)]), and since
> RS may not appear in JSON texts in any other form, RS unambiguously
> delimits the start of any element in the sequence.
> RS is sufficient to unambiguously delimit all top-level JSON value
> types other than numbers.


The Unicode codepoint of the ASCII record separator (RS) character
is U+001E.


Together, the fragments of a metadata stream contain all the information
about the [data streams](#ds) of one or more [traces](#trace).


 A *fragment* is a JSON object; its allowed properties depend on
its `type` property.


| Name | Type | Description | Required? | Default |
|---|---|---|---|---|
| `type` | JSON string | Type of ***F***. The value of this property *MUST* be one of: `"preamble"` ***F*** is a [preamble fragment](#preamble-frag). `"field-class-alias"` ***F*** is a [field class alias fragment](#fca-frag). `"trace-class"` ***F*** is a [trace class fragment](#tc-frag). `"clock-class"` ***F*** is a [clock class fragment](#cc-frag). `"data-stream-class"` ***F*** is a [data stream class fragment](#dsc-frag). `"event-record-class"` ***F*** is a [event record class fragment](#erc-frag). | Yes |  |
| `attributes` | [Attributes](#attrs) | Attributes of ***F***. | No | `{}` |
| `extensions` | [Extensions](#ext) | Extensions of ***F***. For any fragment except a [preamble fragment](#preamble-frag), any extension which exists under this property *MUST* also be declared in the preamble fragment of the same metadata stream. | No | `{}` |


The metadata stream is a JSON text sequence of fragments instead of a
single JSON object containing nested objects to enable real-time, or
“live”, tracing: during tracing, a [consumer](#consumer-def) can
always decode [event records](#er) having known [event record
classes](#erc-frag) while a [producer](#producer-def) can always add new event
record classes to a [data stream class](#dsc-frag) by appending
additional fragments to the metadata stream. Once a producer appends a
fragment to a metadata stream, the fragment is considered “frozen”, in
that it never needs to change.


A metadata stream:


- *MUST* start with a preamble fragment.

- *MUST* contain exactly one [preamble fragment](#preamble-frag).

- *MAY* contain one or more [field class alias fragments](#fca-frag).

- *MAY* contain one [trace class fragment](#tc-frag).

- *MUST* contain one or more [data stream class fragments](#dsc-frag)
which, if any trace class fragment ***T*** exists, *MUST*
follow ***T***.

- *MAY* contain one or more [event record class fragments](#erc-frag)
which, if their parent data stream class ***D*** exists,
*MUST* follow ***D***.


Example 1. Partial metadata stream.


In the sample below, the “␞” symbol represents a single “record
separator (RS)” (U+001E) codepoint and the “…” symbol
represents continuation.


```
␞{
  "type": "preamble",
  "version": 2
}
␞…
```


| * | This section doesn’t specify how a metadata stream translates into [data stream](#ds) encoding and decoding rules; it only describes objects and their properties. See [Data stream decoding procedure](#ds-dec) to learn how to decode a data stream. |
|---|---|


### 5.1. Extensions


A [producer](#producer-def) *MAY* add *extensions* to many metadata
stream JSON objects.


The purpose of an extension is to add core features to CTF 2 or to
modify existing core features, as specified by this document. In other
words, an extension *MAY* **alter** the format itself.


This document doesn’t specify what an extension exactly is.


The [preamble fragment](#preamble-frag) of the metadata stream contains
*extension declarations*:


- Any extension in metadata stream objects *MUST* be declared, by
namespace and name, in the preamble fragment.

Declaring an extension is said to *enable* it.

- If a [consumer](#consumer-def) doesn’t support *any* declared
extension, it *MUST NOT* consume the [data streams](#ds) of the
[trace](#trace).

The consumer *SHOULD* report unsupported extensions as an error.


Extensions are a single JSON object, where each property is:


| Name | A [namespace](#ns-def) |
|---|---|
| Value | A [namespaced extensions object](#ns-exts-obj) |


 A *namespaced extensions object* is a JSON object, where
each property is:


| Name | An extension name |
|---|---|
| Value | A JSON value |


The metadata stream JSON objects which *MAY* contain extensions as their
`extensions` property are:


- Any [fragment](#frag).

An extension in the [preamble fragment](#preamble-frag) also makes it
*declared*/*enabled*.

- Any [field class](#fc).

- A [structure field member class](#struct-member-cls).

- A [variant field class option](#var-fc-opt).


Example 2. Three extensions under two namespaces.


```
{
  "my.tracer": {
    "piano": {
      "keys": 88,
      "temperament": "equal"
    },
    "ramen": 23
  },
  "abc/xyz": {
    "sax": {
      "variant": "alto"
    }
  }
}
```


### 5.2. Attributes


A [producer](#producer-def) *MAY* add custom *attributes* to many
metadata stream JSON objects.


This document doesn’t specify what an attribute exactly is.


Unlike [extensions](#ext), a [consumer](#consumer-def) *MUST NOT*
consider attributes to successfully decode [data streams](#ds).


Attributes are a single JSON object, where each property is:


| Name | A [namespace](#ns-def) |
|---|---|
| Value | A JSON value |


The metadata stream JSON objects which *MAY* contain attributes as their
`attributes` property are:


- Any [fragment](#frag).

- Any [field class](#fc).

- A [structure field member class](#struct-member-cls).

- A [variant field class option](#var-fc-opt).


Example 3. Attributes under two namespaces.


```
{
  "my.tracer": {
    "max-count": 45,
    "module": "sys"
  },
  "abc/xyz": true
}
```


### 5.3. Field classes


A *field class* describes fields, that is, [sequences](#seq-def) of bits
as found in a [data stream](#ds).


A field class contains all the properties a [consumer](#consumer-def)
needs to [decode](#ds-dec) a given field.


A *field* is a field class instance.


This document specifies the following types of field classes:


Abstract field classes

One cannot use the following field classes directly: they are bases
for other, concrete field classes:


- [Abstract integer field class](#int-fc)

- [Abstract string field class](#str-fc)

- [Abstract array field class](#array-fc)

- [Abstract BLOB field class](#blob-fc)


Fixed/static-length scalar field classes


- [Fixed-length bit array field class](#fl-ba-fc)

- [Fixed-length bit map field class](#fl-bm-fc)

- [Fixed-length boolean field class](#fl-bool-fc)

- [Fixed-length integer field class](#fl-int-fc)

- [Fixed-length floating point number field class](#fl-fp-fc)

- [Static-length string field class](#sl-str-fc)

- [Static-length BLOB field class](#sl-blob-fc)


Variable/dynamic-length scalar field classes


- [Variable-length integer field class](#vl-int-fc)

- [Null-terminated string field class](#nt-str-fc)

- [Dynamic-length string field class](#dl-str-fc)

- [Dynamic-length BLOB field class](#dl-blob-fc)


Compound field classes

The following field classes *MAY* contain other field classes:


- [Structure field class](#struct-fc)

- [Static-length array field class](#sl-array-fc)

- [Dynamic-length array field class](#dl-array-fc)

- [Optional field class](#opt-fc)

- [Variant field class](#var-fc)


The following diagram shows the relations between field classes; a field
class box ***A*** containing another field class
box ***B*** means ***B*** is an ***A***:


*


A field class is a JSON object; its properties depend on its `type`
property.


| Name | Type | Description | Required? | Default |
|---|---|---|---|---|
| `type` | JSON string | Type of ***F***. The value of this property *MUST* be one of: `"fixed-length-bit-array"` ***F*** is a [fixed-length bit array field class](#fl-ba-fc). `"fixed-length-bit-map"` ***F*** is a [fixed-length bit map field class](#fl-bm-fc). `"fixed-length-boolean"` ***F*** is a [fixed-length boolean field class](#fl-bool-fc). `"fixed-length-unsigned-integer"` `"fixed-length-signed-integer"` ***F*** is a [fixed-length integer field class](#fl-int-fc). `"fixed-length-floating-point-number"` ***F*** is a [fixed-length floating point number field class](#fl-fp-fc). `"variable-length-unsigned-integer"` `"variable-length-signed-integer"` ***F*** is a [variable-length integer field class](#vl-int-fc). `"null-terminated-string"` ***F*** is a [null-terminated string field class](#nt-str-fc). `"static-length-string"` ***F*** is a [static-length string field class](#sl-str-fc). `"static-length-blob"` ***F*** is a [static-length BLOB field class](#sl-blob-fc). `"dynamic-length-string"` ***F*** is a [dynamic-length string field class](#dl-str-fc). `"dynamic-length-blob"` ***F*** is a [dynamic-length BLOB field class](#dl-blob-fc). `"structure"` ***F*** is a [structure field class](#struct-fc). `"static-length-array"` ***F*** is a [static-length array field class](#sl-array-fc). `"dynamic-length-array"` ***F*** is a [dynamic-length array field class](#dl-array-fc). `"optional"` ***F*** is a [optional field class](#opt-fc). `"variant"` ***F*** is a [variant field class](#var-fc). | Yes |  |
| `attributes` | [Attributes](#attrs) | Attributes of ***F***. | No | `{}` |
| `extensions` | [Extensions](#ext) | Extensions of ***F***. Any extension which exists under this property *MUST* also be declared in the [preamble fragment](#preamble-frag) of the metadata stream. | No | `{}` |


The following [fragment](#frag) properties *MUST* have a [structure field class](#struct-fc)
(or the name of a structure [field class alias](#fca-frag)) as
their value:


[Trace class fragment](#tc-frag)

`packet-header-field-class`


[Data stream class fragment](#dsc-frag)


- `packet-context-field-class`

- `event-record-header-field-class`

- `event-record-common-context-field-class`


[Event record class fragment](#erc-frag)


- `specific-context-field-class`

- `payload-field-class`


#### 5.3.1. Field location


A *field location* is a means for a [consumer](#consumer-def) to locate
a field which it needs to [decode](#ds-dec) another, subsequent field.


A consumer needs to locate another field to decode instances of the
following [classes](#fc):


[Dynamic-length array field class](#dl-array-fc)
[Dynamic-length string field class](#dl-str-fc)
[Dynamic-length BLOB field class](#dl-blob-fc)

Needs a [fixed-length unsigned integer](#fl-int-fc) or
[variable-length unsigned integer](#vl-int-fc) length field.


[Optional field class](#opt-fc)

Needs a [fixed-length boolean](#fl-bool-fc), [fixed-length integer](#fl-int-fc), or
[variable-length integer](#vl-int-fc) selector field.


[Variant field class](#var-fc)

Needs a [fixed-length integer](#fl-int-fc) or [variable-length integer](#vl-int-fc) selector
field.


Let ***T*** be an anteriorly decoded field which a consumer needs in
order to decode another field ***S***.


A field location is a JSON object.


| Name | Type | Description | Required? | Default |
|---|---|---|---|---|
| `origin` | JSON string | Known origin of ***F***, from where to start the [field location procedure](#field-loc-dec) amongst: `"packet-header"` [Header](#pkt-header) of the [packet](#pkt) of ***S***. `"packet-context"` [Context](#pkt-ctx) of the packet of ***S***. `"event-record-header"` [Header](#er-header) of the [event record](#er) of ***S***. `"event-record-common-context"` [Common context](#er-common-ctx) of the event record of ***S***. `"event-record-specific-context"` [Specific context](#er-spec-ctx) of the event record of ***S***. `"event-record-payload"` [Payload](#er-payload) of the event record of ***S***. In other words, ***T*** *MUST* be in the same packet or event record as ***S***. | No | Start the field location procedure from the structure field containing ***S***. |
| `path` | JSON array, each element being one of: A JSON string `null` | Each element is one of, from the origin of ***F***: A JSON string The name of a [structure](#struct-fc) field member to follow to locate  ***T***. `null` The parent structure field of the current (in the field location procedure context) structure field. The value of this property *MUST NOT* be empty. The last element of this property *MUST NOT* be `null`. | Yes |  |


Example 4. Field location with a known origin.


```
{
  "origin": "event-record-header",
  "path": ["id"]
}
```


Example 5. Field location without a known origin.


```
{
  "path": ["streak", "free"]
}
```


Example 6. Field location without a known origin, using a parent structure field.


```
{
  "path": [null, null, "mini", "parking", "stop"]
}
```


#### 5.3.2. Integer range set


An *integer range set* is a JSON array of integer ranges.


An integer range set *MUST* contain one or more integer ranges.


An *integer range* is a JSON array of two elements:


- The lower bound of the range (JSON integer, included).

- The upper bound of the range (JSON integer, included).


An integer range represents all the integer values from the lower bound
of the range to its upper bound.


The upper bound of an integer range *MUST* be greater than or equal to
its lower bound.


If both the lower and upper bounds of an integer range are equal, then
the integer range represents a single integer value.


Example 7. Integer ranges.


```
[3, 67]
```


```
[-45, 101]
```


Single integer value.


```
[42, 42]
```


Example 8. Integer range set containing three integer ranges.


```
[[3, 67], [-45, 1], [42, 42]]
```


#### 5.3.3. Roles


Some [fixed-length unsigned integer](#fl-int-fc), [variable-length unsigned integer](#vl-int-fc), and
[static-length BLOB field class](#sl-blob-fc) instances can have *roles*.


A role is specific semantics attached to the fields (instances) of a
field class. For example, the `packet-magic-number` role of a
fixed-length unsigned integer field class indicates that the value of its instances *MUST*
be the [packet](#pkt) magic number (0xc1fc1fc1).


Roles are a JSON array of role names (JSON strings).


See [Trace class fragment](#tc-frag) and [Data stream class fragment](#dsc-frag) which indicate accepted roles within
their root field classes.


#### 5.3.4. Fixed-length bit array field class


A *fixed-length bit array* field class describes *fixed-length bit array* fields and is a base of a
[fixed-length bit map field class](#fl-bm-fc), a [fixed-length boolean field class](#fl-bool-fc), a [fixed-length integer field class](#fl-int-fc), and a [fixed-length floating point number field class](#fl-fp-fc).


A fixed-length bit array field is a simple array of contiguous bits, without any
attached integer type semantics.


The length, or number of bits, of a fixed-length bit array field is a property
(`length`) of its class.


| Name | Type | Description | Required? | Default |
|---|---|---|---|---|
| `type` | JSON string | Type of ***F***. The value of this property *MUST* be `"fixed-length-bit-array"`. | Yes |  |
| `length` | JSON integer | Number of bits of an instance of ***F***. The value of this property *MUST* be greater than zero. | Yes |  |
| `byte-order` | JSON string | [Byte order](https://en.wikipedia.org/wiki/Endianness) of an instance of ***F***. The value of this property *MUST* be one of: `"big-endian"` Big-endian. Within a [data stream](#ds) [byte](#byte-def), the bits of an instance of ***F*** are decoded from the most significant to the least significant bits of the byte. `"little-endian"` Little-endian. Within a data stream byte, the bits of an instance of ***F*** are decoded from the least significant to the most significant bits of the byte. | Yes |  |
| `bit-order` | JSON string | Bit order of an instance of ***F***. The value of this property *MUST* be one of: `"first-to-last"` The first bit to decode of an instance of ***F*** is the *first* element of the resulting array value. `"last-to-first"` The first bit to decode of an instance of ***F*** is the *last* element of the resulting array value. | No | `"first-to-last"` if the value of the `byte-order` property of ***F*** is `"little-endian"`, or `"last-to-first"` otherwise. |
| `alignment` | JSON integer | Alignment of the first bit of an instance of ***F*** relative to the beginning of the [packet](#pkt) which contains this instance. The value of this property *MUST* be a positive power of two. | No | `1` |
| `attributes` | [Attributes](#attrs) | Attributes of ***F***. | No | `{}` |
| `extensions` | [Extensions](#ext) | Extensions of ***F***. Any extension which exists under this property *MUST* also be declared in the [preamble fragment](#preamble-frag) of the metadata stream. | No | `{}` |


Example 9. Minimal fixed-length bit array field class.


```
{
  "type": "fixed-length-bit-array",
  "length": 16,
  "byte-order": "little-endian"
}
```


Example 10. Fixed-length bit array field class with instances aligned to 32 bits.


```
{
  "type": "fixed-length-bit-array",
  "length": 48,
  "byte-order": "big-endian",
  "alignment": 32
}
```


Example 11. Fixed-length bit array field class with a non-default bit order.


```
{
  "type": "fixed-length-bit-array",
  "length": 32,
  "byte-order": "little-endian",
  "bit-order": "last-to-first"
}
```


Example 12. Fixed-length bit array field class with [attributes](#attrs).


```
{
  "type": "fixed-length-bit-array",
  "length": 16,
  "byte-order": "little-endian",
  "attributes": {
    "my.tracer": {
      "is-nice": true
    }
  }
}
```


#### 5.3.5. Fixed-length bit map field class


A *fixed-length bit map* field class is a [fixed-length bit array field class](#fl-ba-fc) which describes *fixed-length bit map*
fields.


A fixed-length bit map field is a fixed-length bit array field with one or more associated bit names
thanks to the `flags` property of its class.


| Name | Type | Description | Required? | Default |
|---|---|---|---|---|
| `type` | JSON string | Type of ***F***. The value of this property *MUST* be `"fixed-length-bit-map"`. | Yes |  |
| `length` | JSON integer | Number of bits of an instance of ***F***. The value of this property *MUST* be greater than zero. Property inherited from the [fixed-length bit array field class](#fl-ba-fc). | Yes |  |
| `byte-order` | JSON string | [Byte order](https://en.wikipedia.org/wiki/Endianness) of an instance of ***F***. The value of this property *MUST* be one of: `"big-endian"` Big-endian. Within a [data stream](#ds) [byte](#byte-def), the bits of an instance of ***F*** are decoded from the most significant to the least significant bits of the byte. `"little-endian"` Little-endian. Within a data stream byte, the bits of an instance of ***F*** are decoded from the least significant to the most significant bits of the byte. Property inherited from the [fixed-length bit array field class](#fl-ba-fc). | Yes |  |
| `bit-order` | JSON string | Bit order of an instance of ***F***. The value of this property *MUST* be one of: `"first-to-last"` The first bit to decode of an instance of ***F*** is the *first* element of the resulting array value. `"last-to-first"` The first bit to decode of an instance of ***F*** is the *last* element of the resulting array value. Property inherited from the [fixed-length bit array field class](#fl-ba-fc). | No | `"first-to-last"` if the value of the `byte-order` property of ***F*** is `"little-endian"`, or `"last-to-first"` otherwise. |
| `alignment` | JSON integer | Alignment of the first bit of an instance of ***F*** relative to the beginning of the [packet](#pkt) which contains this instance. The value of this property *MUST* be a positive power of two. Property inherited from the [fixed-length bit array field class](#fl-ba-fc). | No | `1` |
| `flags` | [Bit map field class flags](#fl-bm-fc-flags) | Flags of ***F***. The value of this property *MUST* contain one or more properties. | Yes |  |
| `attributes` | [Attributes](#attrs) | Attributes of ***F***. | No | `{}` |
| `extensions` | [Extensions](#ext) | Extensions of ***F***. Any extension which exists under this property *MUST* also be declared in the [preamble fragment](#preamble-frag) of the metadata stream. | No | `{}` |


Example 13. Fixed-length bit map field class with a single flag.


```
{
  "type": "fixed-length-bit-map",
  "length": 16,
  "byte-order": "little-endian",
  "flags": {
    "FEATURE": [[3, 3]]
  }
}
```


Example 14. Fixed-length bit map field class with instances aligned to 32 bits and three flags.


```
{
  "type": "fixed-length-bit-map",
  "length": 48,
  "byte-order": "big-endian",
  "alignment": 32,
  "flags": {
    "shock": [[3, 3]],
    "cord": [[4, 4], [8, 10]],
    "learn": [[2, 7]]
  }
}
```


Example 15. Fixed-length bit map field class with [attributes](#attrs).


```
{
  "type": "fixed-length-bit-map",
  "length": 16,
  "byte-order": "little-endian",
  "flags": {
    "FEATURE": [[3, 3]]
  },
  "attributes": {
    "my.tracer": {
      "is-nice": true
    }
  }
}
```


##### 5.3.5.1. Bit map field class flags


*Bit map field class flags* map names to bit index range sets.


Bit map field class flags are a JSON object, where each property is:


| Name | Flag name. |
|---|---|
| Value | Flag ranges of bit indexes ([integer range set](#int-range-set)). * | The indexes refer to the decoded bit array indexes. |


The bit index ranges of two given flags *MAY* overlap.


Bit map field class flags *MUST* contain one or more properties.


Let ***F*** be some [fixed-length bit map field class](#fl-bm-fc), ***K*** be the value of the
`flags` property of ***F***, and ***V*** be some
[decoded value](#fl-bm-field-dec) of ***F***, then if element (bit)
 ***I*** of ***V*** is *true* and  ***I*** is
an element of one of the ranges of a property ***E***
of ***K***, then the name of ***E*** is said to be
*active* for ***V***.


Example 16. Bit map field class flags with four flags.


In this example, for the bit array value


```
[false, true, false, false, false, true, false, true]
```


the active flag names are `Mercury`, `Earth`, and `Mars`.


```
{
  "Mercury": [[7, 7]],
  "Venus": [[6, 6], [2, 3]],
  "Earth": [[5, 7]],
  "Mars": [[0, 1]]
}
```


#### 5.3.6. Fixed-length boolean field class


A *fixed-length boolean* field class is a [fixed-length bit array field class](#fl-ba-fc) which describes *fixed-length boolean*
fields.


A fixed-length boolean field is a fixed-length bit array field which has the following semantics:


If all the bits of the field are cleared (zero)

The value of the fixed-length boolean field is *false*.


Otherwise

The value of the fixed-length boolean field is *true*.


| Name | Type | Description | Required? | Default |
|---|---|---|---|---|
| `type` | JSON string | Type of ***F***. The value of this property *MUST* be `"fixed-length-boolean"`. | Yes |  |
| `length` | JSON integer | Number of bits of an instance of ***F***. The value of this property *MUST* be greater than zero. Property inherited from the [fixed-length bit array field class](#fl-ba-fc). | Yes |  |
| `byte-order` | JSON string | [Byte order](https://en.wikipedia.org/wiki/Endianness) of an instance of ***F***. The value of this property *MUST* be one of: `"big-endian"` Big-endian. Within a [data stream](#ds) [byte](#byte-def), the bits of an instance of ***F*** are decoded from the most significant to the least significant bits of the byte. `"little-endian"` Little-endian. Within a data stream byte, the bits of an instance of ***F*** are decoded from the least significant to the most significant bits of the byte. The value of this property has no effect on the [decoded value](#fl-bool-field-dec) of an instance of ***F***. Property inherited from the [fixed-length bit array field class](#fl-ba-fc). | Yes |  |
| `bit-order` | JSON string | Bit order of an instance of ***F***. The value of this property *MUST* be one of: `"first-to-last"` The first bit to decode of an instance of ***F*** is the *first* element of the resulting array value. `"last-to-first"` The first bit to decode of an instance of ***F*** is the *last* element of the resulting array value. The value of this property has no effect on the [decoded value](#fl-bool-field-dec) of an instance of ***F***. Property inherited from the [fixed-length bit array field class](#fl-ba-fc). | No | `"first-to-last"` if the value of the `byte-order` property of ***F*** is `"little-endian"`, or `"last-to-first"` otherwise. |
| `alignment` | JSON integer | Alignment of the first bit of an instance of ***F*** relative to the beginning of the [packet](#pkt) which contains this instance. The value of this property *MUST* be a positive power of two. Property inherited from the [fixed-length bit array field class](#fl-ba-fc). | No | `1` |
| `attributes` | [Attributes](#attrs) | Attributes of ***F***. | No | `{}` |
| `extensions` | [Extensions](#ext) | Extensions of ***F***. Any extension which exists under this property *MUST* also be declared in the [preamble fragment](#preamble-frag) of the metadata stream. | No | `{}` |


Example 17. Minimal fixed-length boolean field class.


```
{
  "type": "fixed-length-boolean",
  "length": 16,
  "byte-order": "little-endian"
}
```


Example 18. Fixed-length boolean field class with instances aligned to 32 bits.


```
{
  "type": "fixed-length-boolean",
  "length": 48,
  "byte-order": "big-endian",
  "alignment": 32
}
```


Example 19. Fixed-length boolean field class with [attributes](#attrs).


```
{
  "type": "fixed-length-boolean",
  "length": 16,
  "byte-order": "little-endian",
  "attributes": {
    "my.tracer": {
      "is-nice": true
    }
  }
}
```


#### 5.3.7. Abstract integer field class


An *abstract integer* field class is a base of a [fixed-length integer field class](#fl-int-fc) and a
[variable-length integer field class](#vl-int-fc).


This field class is abstract in that it only exists to show the relation
between different integer field classes in this document: a
[packet](#pkt) cannot contain an abstract integer field.


| Name | Type | Description | Required? | Default |
|---|---|---|---|---|
| `preferred-display-base` | JSON integer | Preferred base to display the value of an instance of ***F***. The value of this property *MUST* be one of: `2` | Binary base. |
| `8` | Octal base. |
| `10` | Decimal base. |
| `16` | Hexadecimal base. |


This property exists to remain backward compatible with [CTF 1](https://diamon.org/ctf/v1.8.3/):
it’s not strictly needed to decode an instance of ***F***
and has no semantic value.


No


`10`


`mappings`


[Integer field class mappings](#int-fc-mappings)


Mappings of ***F***.


No


`{}`


##### 5.3.7.1. Integer field class mappings


*Integer field class mappings* map names to
[integer range sets](#int-range-set).


Integer field class mappings are a JSON object, where each property
is:


| Name | Mapping name. |
|---|---|
| Value | Mapped ranges of integers ([integer range set](#int-range-set)). |


The integer ranges of two given mappings *MAY* overlap.


Example 20. Integer field class mappings with three mappings.


In this example, the `fortune` and `building` mappings overlap with the
values 4 and 5, and the `building` and `journal` mappings
overlap with the value 80.


```
{
  "fortune": [[3, 67], [-45, 1], [84, 84]],
  "building": [[4, 5], [75, 82]],
  "journal": [[100, 2305], [80, 80]]
}
```


#### 5.3.8. Fixed-length integer field class


A *fixed-length integer* field class is both an [abstract integer field
class](#int-fc) and a [fixed-length bit array field class](#fl-ba-fc) which describes *fixed-length integer* fields.


A fixed-length integer field is a fixed-length bit array field which has integer semantics.


If the value of the `type` property of a fixed-length integer is
`"fixed-length-signed-integer"`, then its instances follow the two’s
complement format.


| Name | Type | Description | Required? | Default |
|---|---|---|---|---|
| `type` | JSON string | Type of ***F***. The value of this property *MUST* be one of: `"fixed-length-unsigned-integer"` The instances of ***F*** are fixed-length unsigned integer fields. `"fixed-length-signed-integer"` The instances of ***F*** are fixed-length signed integer fields. | Yes |  |
| `length` | JSON integer | Number of bits of an instance of ***F***. The value of this property *MUST* be greater than zero. Property inherited from the [fixed-length bit array field class](#fl-ba-fc). | Yes |  |
| `byte-order` | JSON string | [Byte order](https://en.wikipedia.org/wiki/Endianness) of an instance of ***F***. The value of this property *MUST* be one of: `"big-endian"` Big-endian. Within a [data stream](#ds) [byte](#byte-def), the bits of an instance of ***F*** are decoded from the most significant to the least significant bits of the byte. `"little-endian"` Little-endian. Within a data stream byte, the bits of an instance of ***F*** are decoded from the least significant to the most significant bits of the byte. Property inherited from the [fixed-length bit array field class](#fl-ba-fc). | Yes |  |
| `bit-order` | JSON string | Bit order of an instance of ***F***. The value of this property *MUST* be one of: `"first-to-last"` The first bit to decode of an instance of ***F*** is the *first* element (least significant bit of the encoded integer value) of the resulting array value. `"last-to-first"` The first bit to decode of an instance of ***F*** is the *last* element (most significant bit of the encoded integer value) of the resulting array value. Property inherited from the [fixed-length bit array field class](#fl-ba-fc). | No | `"first-to-last"` if the value of the `byte-order` property of ***F*** is `"little-endian"`, or `"last-to-first"` otherwise. |
| `alignment` | JSON integer | Alignment of the first bit of an instance of ***F*** relative to the beginning of the [packet](#pkt) which contains this instance. The value of this property *MUST* be a positive power of two. Property inherited from the [fixed-length bit array field class](#fl-ba-fc). | No | `1` |
| `preferred-display-base` | JSON integer | Preferred base to display the value of an instance of ***F***. The value of this property *MUST* be one of: `2` | Binary base. |
| `8` | Octal base. |
| `10` | Decimal base. |
| `16` | Hexadecimal base. |


This property exists to remain backward compatible with [CTF 1](https://diamon.org/ctf/v1.8.3/):
it’s not strictly needed to decode an instance of ***F***
and has no semantic value.


Property inherited from the [abstract integer field class](#int-fc).


No


`10`


`mappings`


[Integer field class mappings](#int-fc-mappings)


Mappings of ***F***.


Property inherited from the [abstract integer field class](#int-fc).


No


`{}`


`roles`


[Roles](#roles)


Roles of an instance of ***F***.


See [Trace class fragment](#tc-frag) and [Data stream class fragment](#dsc-frag) which indicate accepted roles within
their root field classes.


This property *MAY* only exist when the `type` property of ***F***
is `"fixed-length-unsigned-integer"`.


No


`[]`


`attributes`


[Attributes](#attrs)


Attributes of ***F***.


No


`{}`


`extensions`


[Extensions](#ext)


Extensions of ***F***.


Any extension which exists under this property *MUST* also be declared
in the [preamble fragment](#preamble-frag) of the metadata stream.


No


`{}`


Example 21. Minimal fixed-length unsigned integer field class.


```
{
  "type": "fixed-length-unsigned-integer",
  "length": 16,
  "byte-order": "little-endian"
}
```


Example 22. Fixed-length signed integer field class with instances aligned to 32 bits.


```
{
  "type": "fixed-length-signed-integer",
  "length": 48,
  "byte-order": "big-endian",
  "alignment": 32
}
```


Example 23. Fixed-length unsigned integer field class with instances to be preferably displayed with a hexadecimal base.


```
{
  "type": "fixed-length-unsigned-integer",
  "length": 48,
  "byte-order": "big-endian",
  "preferred-display-base": 16
}
```


Example 24. Fixed-length signed integer field class with two mappings.


```
{
  "type": "fixed-length-signed-integer",
  "length": 32,
  "byte-order": "little-endian",
  "mappings": {
    "banana": [[-27399, -1882], [8, 199], [101, 101]],
    "orange": [[67, 67], [43, 1534]]
  }
}
```


Example 25. Fixed-length signed integer field class with [attributes](#attrs).


```
{
  "type": "fixed-length-signed-integer",
  "length": 16,
  "byte-order": "little-endian",
  "attributes": {
    "my.tracer": {
      "is-nice": true
    }
  }
}
```


#### 5.3.9. Fixed-length floating point number field class


A *fixed-length floating point number* field class is a [fixed-length bit array field class](#fl-ba-fc) which describes *fixed-length floating point number*
fields.


A fixed-length floating point number field is a fixed-length bit array which has floating point number semantics.


A fixed-length floating point number is encoded as per [IEEE 754-2008](https://standards.ieee.org/standard/754-2008.html) binary interchange format.


| Name | Type | Description | Required? | Default |
|---|---|---|---|---|
| `type` | JSON string | Type of ***F***. The value of this property *MUST* be `"fixed-length-floating-point-number"`. | Yes |  |
| `length` | JSON integer | Number of bits of an instance of ***F***. The value of this property *MUST* be one of: `16` The instances of ***F*** are binary16 floating point numbers, as per the [IEEE 754-2008](https://standards.ieee.org/standard/754-2008.html) binary interchange format. `32` The instances of ***F*** are binary32 floating point numbers. `64` The instances of ***F*** are binary64 floating point numbers. `128` The instances of ***F*** are binary128 floating point numbers. ***K***, where ***K*** is greater than 128 and a multiple of 32 The instances of ***F*** are binary***K*** floating point numbers. Property inherited from the [fixed-length bit array field class](#fl-ba-fc). | Yes |  |
| `byte-order` | JSON string | [Byte order](https://en.wikipedia.org/wiki/Endianness) of an instance of ***F***. The value of this property *MUST* be one of: `"big-endian"` Big-endian. Within a [data stream](#ds) [byte](#byte-def), the bits of an instance of ***F*** are decoded from the most significant to the least significant bits of the byte. `"little-endian"` Little-endian. Within a data stream byte, the bits of an instance of ***F*** are decoded from the least significant to the most significant bits of the byte. Property inherited from the [fixed-length bit array field class](#fl-ba-fc). | Yes |  |
| `bit-order` | JSON string | Bit order of an instance of ***F***. The value of this property *MUST* be one of: `"first-to-last"` The first bit to decode of an instance of ***F*** is the *first* element (least significant bit of the mantissa) of the resulting array value. `"last-to-first"` The first bit to decode of an instance of ***F*** is the *last* element (sign bit) of the resulting array value. Property inherited from the [fixed-length bit array field class](#fl-ba-fc). | No | `"first-to-last"` if the value of the `byte-order` property of ***F*** is `"little-endian"`, or `"last-to-first"` otherwise. |
| `alignment` | JSON integer | Alignment of the first bit of an instance of ***F*** relative to the beginning of the [packet](#pkt) which contains this instance. The value of this property *MUST* be a positive power of two. Property inherited from the [fixed-length bit array field class](#fl-ba-fc). | No | `1` |
| `attributes` | [Attributes](#attrs) | Attributes of ***F***. | No | `{}` |
| `extensions` | [Extensions](#ext) | Extensions of ***F***. Any extension which exists under this property *MUST* also be declared in the [preamble fragment](#preamble-frag) of the metadata stream. | No | `{}` |


Example 26. Minimal binary32 fixed-length floating point number field class.


```
{
  "type": "fixed-length-floating-point-number",
  "length": 32,
  "byte-order": "little-endian"
}
```


Example 27. binary64 fixed-length floating point number field class with instances aligned to 32 bits.


```
{
  "type": "fixed-length-floating-point-number",
  "length": 64,
  "byte-order": "big-endian",
  "alignment": 32
}
```


Example 28. binary192 fixed-length floating point number field class with [attributes](#attrs).


```
{
  "type": "fixed-length-floating-point-number",
  "length": 192,
  "byte-order": "little-endian",
  "attributes": {
    "my.tracer": {
      "is-nice": true
    }
  }
}
```


#### 5.3.10. Variable-length integer field class


A *variable-length integer* field class is an [abstract integer field
class](#int-fc) which describes *variable-length integer* fields.


A variable-length integer field is a [sequence](#seq-def) of [bytes](#byte-def) with a
variable length which contains a multiple of 7 effective bits
encoding an unsigned or signed integer value.


A variable-length integer field is encoded as per
[LEB128](https://en.wikipedia.org/wiki/LEB128).


If the value of the `type` property of a variable-length integer field class is
`"variable-length-signed-integer"`, then its instances follow the two’s
complement format.


| Name | Type | Description | Required? | Default |
|---|---|---|---|---|
| `type` | JSON string | Type of ***F***. The value of this property *MUST* be one of: `"variable-length-unsigned-integer"` The instances of ***F*** are variable-length unsigned integer fields. `"variable-length-signed-integer"` The instances of ***F*** are variable-length signed integer fields. | Yes |  |
| `preferred-display-base` | JSON integer | Preferred base to display the value of an instance of ***F***. The value of this property *MUST* be one of: `2` | Binary base. |
| `8` | Octal base. |
| `10` | Decimal base. |
| `16` | Hexadecimal base. |


This property exists to remain backward compatible with [CTF 1](https://diamon.org/ctf/v1.8.3/):
it’s not strictly needed to decode an instance of ***F***
and has no semantic value.


Property inherited from the [abstract integer field class](#int-fc).


No


`10`


`mappings`


[Integer field class mappings](#int-fc-mappings)


Mappings of ***F***.


Property inherited from the [abstract integer field class](#int-fc).


No


`{}`


`roles`


[Roles](#roles)


Roles of an instance of ***F***.


See [Trace class fragment](#tc-frag) and [Data stream class fragment](#dsc-frag) which indicate accepted roles within
their root field classes.


This property *MAY* only exist when the `type` property of ***F***
is `"variable-length-unsigned-integer"`.


No


`[]`


`attributes`


[Attributes](#attrs)


Attributes of ***F***.


No


`{}`


`extensions`


[Extensions](#ext)


Extensions of ***F***.


Any extension which exists under this property *MUST* also be declared
in the [preamble fragment](#preamble-frag) of the metadata stream.


No


`{}`


Example 29. Minimal variable-length unsigned integer field class.


```
{
  "type": "variable-length-unsigned-integer"
}
```


Example 30. Variable-length signed integer field class with instances to be preferably displayed with a hexadecimal base.


```
{
  "type": "variable-length-signed-integer",
  "preferred-display-base": 16
}
```


Example 31. Variable-length unsigned integer field class with three mappings.


```
{
  "type": "variable-length-unsigned-integer",
  "mappings": {
    "lime": [[3, 3]],
    "kiwi": [[8, 8]],
    "blueberry": [[11, 11]]
  }
}
```


Example 32. Variable-length unsigned integer field class with [attributes](#attrs).


```
{
  "type": "variable-length-unsigned-integer",
  "attributes": {
    "my.tracer": {
      "is-nice": true
    }
  }
}
```


#### 5.3.11. Abstract string field class


An *abstract string* field class is a base of a [null-terminated string field class](#nt-str-fc), a
[static-length string field class](#sl-str-fc), and a [dynamic-length string field class](#dl-str-fc).


This field class is abstract in that it only exists to show the relation
between different string field classes in this document: a
[packet](#pkt) cannot contain an abstract string field.


| Name | Type | Description | Required? | Default |
|---|---|---|---|---|
| `encoding` | JSON string | Character encoding of an instance of ***F***. The value of this property *MUST* be one of: `utf-8` | [UTF-8](https://en.wikipedia.org/wiki/UTF-8). |
| `utf-16be` | [UTF-16BE](https://en.wikipedia.org/wiki/UTF-16) (big endian). |
| `utf-16le` | [UTF-16LE](https://en.wikipedia.org/wiki/UTF-16) (little endian). |
| `utf-32be` | [UTF-32BE](https://en.wikipedia.org/wiki/UTF-32) (big endian). |
| `utf-32le` | [UTF-32LE](https://en.wikipedia.org/wiki/UTF-32) (little endian). |


No


`"utf-8"`


#### 5.3.12. Null-terminated string field class


A *null-terminated string* field class is an [abstract string field class](#str-fc)
which describes *null-terminated string* fields.


A null-terminated string field is, in this order:


- Zero or more contiguous encoded Unicode codepoints, following the
`encoding` property of its class.

- One “NULL” (U+0000) codepoint, following the `encoding` property of
its class.


| Name | Type | Description | Required? | Default |
|---|---|---|---|---|
| `type` | JSON string | Type of ***F***. The value of this property *MUST* be `"null-terminated-string"`. | Yes |  |
| `encoding` | JSON string | Character encoding of an instance of ***F***. The value of this property *MUST* be one of: `utf-8` | [UTF-8](https://en.wikipedia.org/wiki/UTF-8). |
| `utf-16be` | [UTF-16BE](https://en.wikipedia.org/wiki/UTF-16) (big endian). |
| `utf-16le` | [UTF-16LE](https://en.wikipedia.org/wiki/UTF-16) (little endian). |
| `utf-32be` | [UTF-32BE](https://en.wikipedia.org/wiki/UTF-32) (big endian). |
| `utf-32le` | [UTF-32LE](https://en.wikipedia.org/wiki/UTF-32) (little endian). |


Property inherited from the [abstract string field class](#str-fc).


No


`"utf-8"`


`attributes`


[Attributes](#attrs)


Attributes of ***F***.


No


`{}`


`extensions`


[Extensions](#ext)


Extensions of ***F***.


Any extension which exists under this property *MUST* also be declared
in the [preamble fragment](#preamble-frag) of the metadata stream.


No


`{}`


Example 33. Minimal UTF-8 null-terminated string field class.


```
{
  "type": "null-terminated-string"
}
```


Example 34. UTF-16LE null-terminated string field class.


```
{
  "type": "null-terminated-string",
  "encoding": "utf-16le"
}
```


Example 35. Null-terminated string field class with [attributes](#attrs).


```
{
  "type": "null-terminated-string",
  "attributes": {
    "my.tracer": {
      "is-nice": true
    }
  }
}
```


#### 5.3.13. Static-length string field class


A *static-length string* field class is an [abstract string field class](#str-fc)
which describes *static-length string* fields.


A static-length string field is a [sequence](#seq-def) of zero or more contiguous
encoded Unicode codepoints. All the encoded codepoints of a static-length string
field before the first “NULL” (U+0000) codepoint, if any, form the
resulting string value. The first U+0000 codepoint, if any, and all the
following bytes are considered padding (garbage data).


The length, or number of *bytes*, of a static-length string field is a property
(`length`) of its class.


| Name | Type | Description | Required? | Default |
|---|---|---|---|---|
| `type` | JSON string | Type of ***F***. The value of this property *MUST* be `"static-length-string"`. | Yes |  |
| `length` | JSON integer | Number of *bytes* contained in an instance of ***F***. In general, this doesn’t mean the number of encoded Unicode codepoints. The value of this property *MUST* be greater than or equal to zero. | Yes |  |
| `encoding` | JSON string | Character encoding of an instance of ***F***. The value of this property *MUST* be one of: `utf-8` | [UTF-8](https://en.wikipedia.org/wiki/UTF-8). |
| `utf-16be` | [UTF-16BE](https://en.wikipedia.org/wiki/UTF-16) (big endian). |
| `utf-16le` | [UTF-16LE](https://en.wikipedia.org/wiki/UTF-16) (little endian). |
| `utf-32be` | [UTF-32BE](https://en.wikipedia.org/wiki/UTF-32) (big endian). |
| `utf-32le` | [UTF-32LE](https://en.wikipedia.org/wiki/UTF-32) (little endian). |


Property inherited from the [abstract string field class](#str-fc).


No


`"utf-8"`


`attributes`


[Attributes](#attrs)


Attributes of ***F***.


No


`{}`


`extensions`


[Extensions](#ext)


Extensions of ***F***.


Any extension which exists under this property *MUST* also be declared
in the [preamble fragment](#preamble-frag) of the metadata stream.


No


`{}`


Example 36. Empty UTF-8 static-length string field class.


```
{
  "type": "static-length-string",
  "length": 0
}
```


Example 37. UTF-8 static-length string field class with instances having 100 bytes.


```
{
  "type": "static-length-string",
  "length": 100
}
```


Example 38. UTF-32BE static-length string field class with instances having 64 bytes.


```
{
  "type": "static-length-string",
  "encoding": "utf-32be",
  "length": 64
}
```


Example 39. Static-length string field class with [attributes](#attrs).


```
{
  "type": "static-length-string",
  "length": 13,
  "attributes": {
    "my.tracer": null
  }
}
```


#### 5.3.14. Dynamic-length string field class


A *dynamic-length string* field class is an [abstract string field class](#str-fc)
which describes *dynamic-length string* fields.


A dynamic-length string field is a [sequence](#seq-def) of zero or more contiguous
encoded Unicode codepoints. All the encoded codepoints of a dynamic-length string
field before the first “NULL” (U+0000) codepoint, if any, form the
resulting string value. The first U+0000 codepoint, if any, and all the
following bytes are considered padding (garbage data).


The length, or number of *bytes*, of a dynamic-length string field is the value of
another, anterior (already encoded/decoded) *length* field. A
[consumer](#consumer-def) can locate this length field thanks to the
`length-field-location` property of the dynamic-length string field class.


| Name | Type | Description | Required? | Default |
|---|---|---|---|---|
| `type` | JSON string | Type of ***F***. The value of this property *MUST* be `"dynamic-length-string"`. | Yes |  |
| `length-field-location` | [Field location](#field-loc) | Location of an anterior field of which the value is the number of *bytes* contained in an instance of ***F***. In general, this doesn’t mean the number of encoded Unicode codepoints. Any length field *MUST* be an instance of one of: [Fixed-length unsigned integer field class](#fl-int-fc) [Variable-length unsigned integer field class](#vl-int-fc) | Yes |  |
| `encoding` | JSON string | Character encoding of an instance of ***F***. The value of this property *MUST* be one of: `utf-8` | [UTF-8](https://en.wikipedia.org/wiki/UTF-8). |
| `utf-16be` | [UTF-16BE](https://en.wikipedia.org/wiki/UTF-16) (big endian). |
| `utf-16le` | [UTF-16LE](https://en.wikipedia.org/wiki/UTF-16) (little endian). |
| `utf-32be` | [UTF-32BE](https://en.wikipedia.org/wiki/UTF-32) (big endian). |
| `utf-32le` | [UTF-32LE](https://en.wikipedia.org/wiki/UTF-32) (little endian). |


Property inherited from the [abstract string field class](#str-fc).


No


`"utf-8"`


`attributes`


[Attributes](#attrs)


Attributes of ***F***.


No


`{}`


`extensions`


[Extensions](#ext)


Extensions of ***F***.


Any extension which exists under this property *MUST* also be declared
in the [preamble fragment](#preamble-frag) of the metadata stream.


No


`{}`


Example 40. UTF-8 dynamic-length string field class.


```
{
  "type": "dynamic-length-string",
  "length-field-location": {
    "origin": "event-record-payload",
    "path": ["length"]
  }
}
```


Example 41. UTF-16BE dynamic-length string field class.


```
{
  "type": "dynamic-length-string",
  "length-field-location": {
    "path": [null, "attrs", "len"]
  },
  "encoding": "utf-16be"
}
```


Example 42. Dynamic-length string field class with [attributes](#attrs).


```
{
  "type": "dynamic-length-string",
  "length-field-location": {
    "path": ["name-length"]
  },
  "attributes": {
    "my.tracer": 177
  }
}
```


#### 5.3.15. Abstract BLOB field class


An *abstract [BLOB](https://en.wikipedia.org/wiki/Binary_large_object)*
field class is a base of a [static-length BLOB field class](#sl-blob-fc) and a [dynamic-length BLOB field class](#dl-blob-fc).


This field class is abstract in that it only exists to show the relation
between different BLOB field classes in this document: a [packet](#pkt)
cannot contain an abstract BLOB field.


| Name | Type | Description | Required? | Default |
|---|---|---|---|---|
| `media-type` | JSON string | [IANA media type](https://datatracker.ietf.org/doc/html/rfc2046) of an instance of ***F***. | No | `"application/octet-stream"` |


#### 5.3.16. Static-length BLOB field class


A *static-length BLOB* field class is an [abstract BLOB field class](#blob-fc)
which describes *static-length BLOB* fields.


A static-length BLOB field is a [sequence](#seq-def) of zero or more contiguous
bytes with an associated IANA media type (given by the `media-type`
property of its class).


The length, or number of bytes, of a static-length BLOB field is a property
(`length`) of its class.


| Name | Type | Description | Required? | Default |
|---|---|---|---|---|
| `type` | JSON string | Type of ***F***. The value of this property *MUST* be `"static-length-blob"`. | Yes |  |
| `length` | JSON integer | Number of bytes contained in an instance of ***F***. The value of this property *MUST* be greater than or equal to zero. | Yes |  |
| `media-type` | JSON string | [IANA media type](https://datatracker.ietf.org/doc/html/rfc2046) of an instance of ***F***. Property inherited from the [abstract BLOB field class](#blob-fc). | No | `"application/octet-stream"` |
| `roles` | [Roles](#roles) | Roles of an instance of ***F***. See [Trace class fragment](#tc-frag) and [Data stream class fragment](#dsc-frag) which indicate accepted roles within their root field classes. | No | `[]` |
| `attributes` | [Attributes](#attrs) | Attributes of ***F***. | No | `{}` |
| `extensions` | [Extensions](#ext) | Extensions of ***F***. Any extension which exists under this property *MUST* also be declared in the [preamble fragment](#preamble-frag) of the metadata stream. | No | `{}` |


Example 43. Empty static-length BLOB field class with instances having a default IANA media type.


```
{
  "type": "static-length-blob",
  "length": 0
}
```


Example 44. Static-length TIFF BLOB field class with instances having 511,267 bytes.


```
{
  "type": "static-length-blob",
  "length": 511267,
  "media-type": "image/tiff"
}
```


Example 45. Static-length CSV BLOB field class with [attributes](#attrs).


```
{
  "type": "static-length-blob",
  "length": 2400,
  "media-type": "text/csv",
  "attributes": {
    "my.tracer": {
      "csv-cols": 12
    }
  }
}
```


#### 5.3.17. Dynamic-length BLOB field class


A *dynamic-length BLOB* field class is an [abstract BLOB field class](#blob-fc)
which describes *dynamic-length BLOB* fields.


A dynamic-length BLOB field is a [sequence](#seq-def) of zero or more contiguous
bytes with an associated IANA media type.


The length, or number of bytes, of a dynamic-length BLOB field is the value of
another, anterior (already encoded/decoded) *length* field. A
[consumer](#consumer-def) can locate this length field thanks to the
`length-field-location` property of the dynamic-length BLOB field class.


| Name | Type | Description | Required? | Default |
|---|---|---|---|---|
| `type` | JSON string | Type of ***F***. The value of this property *MUST* be `"dynamic-length-blob"`. | Yes |  |
| `length-field-location` | [Field location](#field-loc) | Location of an anterior field of which the value is the number of bytes contained in an instance of ***F***. Any length field *MUST* be an instance of one of: [Fixed-length unsigned integer field class](#fl-int-fc) [Variable-length unsigned integer field class](#vl-int-fc) | Yes |  |
| `media-type` | JSON string | [IANA media type](https://datatracker.ietf.org/doc/html/rfc2046) of an instance of ***F***. Property inherited from the [abstract BLOB field class](#blob-fc). | No | `"application/octet-stream"` |
| `attributes` | [Attributes](#attrs) | Attributes of ***F***. | No | `{}` |
| `extensions` | [Extensions](#ext) | Extensions of ***F***. Any extension which exists under this property *MUST* also be declared in the [preamble fragment](#preamble-frag) of the metadata stream. | No | `{}` |


Example 46. Dynamic-length BLOB field class with instances having a default IANA media type.


```
{
  "type": "dynamic-length-blob",
  "length-field-location": {
    "origin": "event-record-payload",
    "path": ["length"]
  }
}
```


Example 47. Dynamic-length JPEG BLOB field class with [attributes](#attrs).


```
{
  "type": "dynamic-length-blob",
  "length-field-location": {
    "path": [null, "length"]
  },
  "media-type": "image/jpeg",
  "attributes": {
    "my.tracer": {
      "quality": 85
    }
  }
}
```


#### 5.3.18. Structure field class


A *structure field class* describes *structure fields*.


A structure field is a [sequence](#seq-def) of zero or more structure
field *members*. A structure field member is a named field.


| Name | Type | Description | Required? | Default |
|---|---|---|---|---|
| `type` | JSON string | Type of ***F***. The value of this property *MUST* be `"structure"`. | Yes |  |
| `member-classes` | JSON array of [structure field member classes](#struct-member-cls) | Classes of the members of an instance of ***F***. The `name` property of each member class *MUST* be unique within the member class names of ***F***. | No | `[]` |
| `minimum-alignment` | JSON integer | Minimum alignment of the first bit of an instance of ***F*** relative to the beginning of the [packet](#pkt) which contains this instance. The value of this property *MUST* be a positive power of two. The [effective alignment](#align-dec) of the first bit of an instance of ***F*** *MAY* be greater than the value of this property. | No | `1` |
| `attributes` | [Attributes](#attrs) | Attributes of ***F***. | No | `{}` |
| `extensions` | [Extensions](#ext) | Extensions of ***F***. Any extension which exists under this property *MUST* also be declared in the [preamble fragment](#preamble-frag) of the metadata stream. | No | `{}` |


Example 48. Empty structure field class: instances have no members.


```
{
  "type": "structure"
}
```


Example 49. Structure field class with three member classes.


```
{
  "type": "structure",
  "member-classes": [
    {
      "name": "Villeray",
      "field-class": {
        "type": "null-terminated-string"
      }
    },
    {
      "name": "Berri",
      "field-class": {
        "type": "fixed-length-unsigned-integer",
        "length": 32,
        "byte-order": "little-endian",
        "preferred-display-base": 2
      },
      "attributes": {
        "my.tracer": {
          "is-mask": true
        }
      }
    },
    {
      "name": "Faillon",
      "field-class": {
        "type": "fixed-length-boolean",
        "length": 8,
        "byte-order": "little-endian"
      }
    }
  ]
}
```


Example 50. Structure field class with instances minimally aligned to 64 bits.


```
{
  "type": "structure",
  "member-classes": [
    {
      "name": "St-Denis",
      "field-class": {
        "type": "null-terminated-string"
      }
    },
    {
      "name": "Lajeunesse",
      "field-class": {
        "type": "fixed-length-unsigned-integer",
        "length": 32,
        "byte-order": "big-endian",
        "alignment": 32
      }
    }
  ],
  "minimum-alignment": 64
}
```


Example 51. Structure field class with [attributes](#attrs).


```
{
  "type": "structure",
  "member-classes": [
    {
      "name": "Henri-Julien",
      "field-class": {
        "type": "fixed-length-signed-integer",
        "length": 48,
        "byte-order": "little-endian"
      }
    },
    {
      "name": "Casgrain",
      "field-class": {
        "type": "static-length-string",
        "length": 32
      }
    }
  ],
  "attributes": {
    "my.tracer": {
      "version": 4
    }
  }
}
```


##### 5.3.18.1. Structure field member class


A *structure field member class* describes *structure field members*.


A structure field member class is a JSON object.


| Name | Type | Description | Required? | Default |
|---|---|---|---|---|
| `name` | JSON string | Name of ***M***. | Yes |  |
| `field-class` | [Field class](#fc) or JSON string | Depending on the type of a value ***V*** of this property: [Field class](#fc) ***V*** is the field class of ***M***. JSON string The effective field class of the previously occurring [field class alias](#fca-frag) named ***V*** is the field class of ***M***. | Yes |  |
| `attributes` | [Attributes](#attrs) | Attributes of ***M***. | No | `{}` |
| `extensions` | [Extensions](#ext) | Extensions of ***M***. Any extension which exists under this property *MUST* also be declared in the [preamble fragment](#preamble-frag) of the metadata stream. | No | `{}` |


Example 52. [Null-terminated string field class](#nt-str-fc) member class named `cat`.


```
{
  "name": "cat",
  "field-class": {
    "type": "null-terminated-string"
  }
}
```


Example 53. [Variable-length signed integer field class](#vl-int-fc) member class named `dog` with [attributes](#attrs).


```
{
  "name": "dog",
  "field-class": {
    "type": "variable-length-signed-integer",
    "preferred-display-base": 8
  },
  "attributes": {
    "my.tracer": {
      "uuid": [
        243, 97, 0, 184, 236, 54, 72, 97,
        141, 107, 169, 214, 171, 137, 115, 201
      ],
      "is-pid": true
    }
  }
}
```


#### 5.3.19. Abstract array field class


An *abstract array* field class is a base of a [static-length array field class](#sl-array-fc) and a
[dynamic-length array field class](#dl-array-fc).


This field class is abstract in that it only exists to show the relation
between different array field classes in this document: a [packet](#pkt)
cannot contain an abstract array field.


| Name | Type | Description | Required? | Default |
|---|---|---|---|---|
| `element-field-class` | [Field class](#fc) or JSON string | Depending on the type of a value ***V*** of this property: [Field class](#fc) ***V*** is the class of the element fields contained in an instance of ***F***. JSON string The effective field class of the previously occurring [field class alias](#fca-frag) named ***V*** is the class of the element fields contained in an instance of ***F***. | Yes |  |
| `minimum-alignment` | JSON integer | Minimum alignment of the first bit of an instance of ***F*** relative to the beginning of the [packet](#pkt) which contains this instance. The value of this property *MUST* be a positive power of two. The [effective alignment](#align-dec) of the first bit of an instance of ***F*** *MAY* be greater than the value of this property. | No | `1` |


#### 5.3.20. Static-length array field class


A *static-length array* field class is an [abstract array field class](#array-fc)
which describes *static-length array* fields.


A static-length array field is a sequence of zero or more element fields.


The length, or number of element fields, of a static-length array field is a
property (`length`) of its class.


| Name | Type | Description | Required? | Default |
|---|---|---|---|---|
| `type` | JSON string | Type of ***F***. The value of this property *MUST* be `"static-length-array"`. | Yes |  |
| `element-field-class` | [Field class](#fc) or JSON string | Depending on the type of a value ***V*** of this property: [Field class](#fc) ***V*** is the class of the element fields contained in an instance of ***F***. JSON string The effective field class of the previously occurring [field class alias](#fca-frag) named ***V*** is the class of the element fields contained in an instance of ***F***. Property inherited from the [abstract array field class](#array-fc). | Yes |  |
| `minimum-alignment` | JSON integer | Minimum alignment of the first bit of an instance of ***F*** relative to the beginning of the [packet](#pkt) which contains this instance. The value of this property *MUST* be a positive power of two. The [effective alignment](#align-dec) of the first bit of an instance of ***F*** *MAY* be greater than the value of this property. Property inherited from the [abstract array field class](#array-fc). | No | `1` |
| `length` | JSON integer | Number of element fields contained in an instance of ***F***. The value of this property *MUST* be greater than or equal to zero. | Yes |  |
| `attributes` | [Attributes](#attrs) | Attributes of ***F***. | No | `{}` |
| `extensions` | [Extensions](#ext) | Extensions of ***F***. Any extension which exists under this property *MUST* also be declared in the [preamble fragment](#preamble-frag) of the metadata stream. | No | `{}` |


Example 54. Empty static-length array field class.


```
{
  "type": "static-length-array",
  "element-field-class": {
    "type": "fixed-length-signed-integer",
    "length": 16,
    "byte-order": "little-endian",
    "alignment": 16
  },
  "length": 0
}
```


Example 55. Static-length array field class with instances having 100 [null-terminated string](#nt-str-fc) fields.


```
{
  "type": "static-length-array",
  "element-field-class": {
    "type": "null-terminated-string"
  },
  "length": 100
}
```


Example 56. Static-length array field class with [attributes](#attrs).


```
{
  "type": "static-length-array",
  "element-field-class": {
    "type": "variable-length-unsigned-integer"
  },
  "length": 13,
  "attributes": {
    "my.tracer": true
  }
}
```


Example 57. Static-length array field class with instances minimally aligned to 32 bits.


With the following static-length array field class, a [producer](#producer-def)
can write a single 32-bit-aligned, 32-bit little-endian integer value,
and have [consumers](#consumer-def) [decode](#sl-array-field-dec) it as
an array of 32 flags (booleans).


```
{
  "type": "static-length-array",
  "length": 32,
  "minimum-alignment": 32,
  "element-field-class": {
    "type": "fixed-length-boolean",
    "length": 1,
    "byte-order": "little-endian"
  }
}
```


#### 5.3.21. Dynamic-length array field class


A *dynamic-length array* field class is an [abstract array field class](#array-fc)
which describes *dynamic-length array* fields.


A dynamic-length array field is a sequence of zero or more element fields.


The length, or number of element fields, of a dynamic-length array field is the
value of another, anterior (already encoded/decoded) *length* field. A
[consumer](#consumer-def) can locate this length field thanks to the
`length-field-location` property of the dynamic-length array field class.


| Name | Type | Description | Required? | Default |
|---|---|---|---|---|
| `type` | JSON string | Type of ***F***. The value of this property *MUST* be `"dynamic-length-array"`. | Yes |  |
| `element-field-class` | [Field class](#fc) or JSON string | Depending on the type of a value ***V*** of this property: [Field class](#fc) ***V*** is the class of the element fields contained in an instance of ***F***. JSON string The effective field class of the previously occurring [field class alias](#fca-frag) named ***V*** is the class of the element fields contained in an instance of ***F***. Property inherited from the [abstract array field class](#array-fc). | Yes |  |
| `minimum-alignment` | JSON integer | Minimum alignment of the first bit of an instance of ***F*** relative to the beginning of the [packet](#pkt) which contains this instance. The value of this property *MUST* be a positive power of two. The [effective alignment](#align-dec) of the first bit of an instance of ***F*** *MAY* be greater than the value of this property. Property inherited from the [abstract array field class](#array-fc). | No | `1` |
| `length-field-location` | [Field location](#field-loc) | Location of an anterior field of which the value is the number of element fields contained in an instance of ***F***. Any length field *MUST* be an instance of one of: [Fixed-length unsigned integer field class](#fl-int-fc) [Variable-length unsigned integer field class](#vl-int-fc) | Yes |  |
| `attributes` | [Attributes](#attrs) | Attributes of ***F***. | No | `{}` |
| `extensions` | [Extensions](#ext) | Extensions of ***F***. Any extension which exists under this property *MUST* also be declared in the [preamble fragment](#preamble-frag) of the metadata stream. | No | `{}` |


Example 58. Dynamic-length array field class.


```
{
  "type": "dynamic-length-array",
  "element-field-class": {
    "type": "fixed-length-unsigned-integer",
    "length": 32,
    "byte-order": "big-endian",
    "alignment": 16
  },
  "length-field-location": {
    "origin": "event-record-payload",
    "path": ["length"]
  }
}
```


Example 59. Dynamic-length array field class with [attributes](#attrs).


```
{
  "type": "dynamic-length-array",
  "element-field-class": {
    "type": "variable-length-unsigned-integer"
  },
  "length-field-location": {
    "path": ["from-user", "common-length"]
  },
  "attributes": {
    "my.tracer": 177
  }
}
```


#### 5.3.22. Optional field class


An *optional* field class describes *optional* fields.


An optional field is, depending on the value of another, anterior
(already encoded/decoded) *selector* field, one of:


- An instance of a given field class (`field-class` property of the
optional field class).

In this case, the optional field is said to be *enabled*.

- A zero-bit field (no field).

In this case, the optional field is said to be *disabled*.


A [consumer](#consumer-def) can locate the selector field thanks to the
`selector-field-location` property of the optional field class.


| Name | Type | Description | Required? | Default |
|---|---|---|---|---|
| `type` | JSON string | Type of ***F***. The value of this property *MUST* be `"optional"`. | Yes |  |
| `field-class` | [Field class](#fc) or JSON string | Depending on the type of a value ***V*** of this property: [Field class](#fc) ***V*** is the class of an instance of ***F*** when it’s enabled. JSON string The effective field class of the previously occurring [field class alias](#fca-frag) named ***V*** is the class of an instance of ***F*** when it’s enabled. | Yes |  |
| `selector-field-location` | [Field location](#field-loc) | Location of an anterior field of which the value indicates whether or not an instance of ***F*** is enabled. A selector field ***S*** *MUST* be an instance of one of: [Fixed-length boolean field class](#fl-bool-fc) An instance of ***F*** is enabled when ***S*** is *true*. [Fixed-length integer field class](#fl-int-fc) [Variable-length integer field class](#vl-int-fc) An instance of ***F*** is enabled when the value of ***S*** is an element of any of the integer ranges of the `selector-field-ranges` property of ***F***. For a given instance of ***F***, the `type` property of the [classes](#fc) of *all* the possible selector fields *MUST* be one of: [`"fixed-length-boolean"`](#fl-bool-fc) Any of: [`"fixed-length-unsigned-integer"`](#fl-int-fc) [`"variable-length-unsigned-integer"`](#vl-int-fc) Any of: [`"fixed-length-signed-integer"`](#fl-int-fc) [`"variable-length-signed-integer"`](#vl-int-fc) | Yes |  |
| `selector-field-ranges` | [Integer range set](#int-range-set) | Ranges of integers which the value of a selector field *MUST* be an element of to enable an instance of ***F***. | Yes, if the selector field is an instance of a [fixed-length integer field class](#fl-int-fc) or a [variable-length integer field class](#vl-int-fc). | None if the selector field is an instance of a [fixed-length boolean field class](#fl-bool-fc). |
| `attributes` | [Attributes](#attrs) | Attributes of ***F***. | No | `{}` |
| `extensions` | [Extensions](#ext) | Extensions of ***F***. Any extension which exists under this property *MUST* also be declared in the [preamble fragment](#preamble-frag) of the metadata stream. | No | `{}` |


Example 60. Optional [static-length array field class](#sl-array-fc) with a [boolean](#fl-bool-fc) selector field class.


```
{
  "type": "optional",
  "selector-field-location": {
    "origin": "event-record-payload",
    "path": ["has-ip"]
  },
  "field-class": {
    "type": "static-length-array",
    "element-field-class": {
      "type": "fixed-length-unsigned-integer",
      "length": 8,
      "byte-order": "little-endian",
      "alignment": 8
    },
    "length": 16
  }
}
```


Example 61. Optional null-terminated string with a [fixed-length signed integer](#fl-int-fc) selector field class.


```
{
  "type": "optional",
  "selector-field-location": {
    "origin": "event-record-payload",
    "path": ["has-ip"]
  },
  "selector-field-ranges": [[-12, -12], [-5, 0], [15, 35]],
  "field-class": {
    "type": "null-terminated-string"
  }
}
```


#### 5.3.23. Variant field class


A *variant* field class describes *variant* fields.


A variant field is, depending on the value of another, anterior (already
encoded/decoded) *selector* field, the instance of a specific, effective
field class amongst one or more *variant field class options*.


A [consumer](#consumer-def) can locate the selector field thanks to the
`selector-field-location` property of the variant field class.


| Name | Type | Description | Required? | Default |
|---|---|---|---|---|
| `type` | JSON string | Type of ***F***. The value of this property *MUST* be `"variant"`. | Yes |  |
| `options` | JSON array of [variant field class options](#var-fc-opt) | Options containing the possible effective classes of an instance of ***F***. This array *MUST* contain one or more elements. The `name` property of each option, if it’s set, *MUST* be unique within the option names of ***F***. The integer ranges (`selector-field-ranges` property) of two given options *MUST NOT* intersect. | Yes |  |
| `selector-field-location` | [Field location](#field-loc) | Location of an anterior field of which the value indicates which option of ***F*** contains the effective class of an instance of ***F***. For a given instance of ***F***, the `type` property of the [classes](#fc) of *all* the possible selector fields *MUST* be one of: Any of: [`"fixed-length-unsigned-integer"`](#fl-int-fc) [`"variable-length-unsigned-integer"`](#vl-int-fc) Any of: [`"fixed-length-signed-integer"`](#fl-int-fc) [`"variable-length-signed-integer"`](#vl-int-fc) | Yes |  |
| `attributes` | [Attributes](#attrs) | Attributes of ***F***. | No | `{}` |
| `extensions` | [Extensions](#ext) | Extensions of ***F***. Any extension which exists under this property *MUST* also be declared in the [preamble fragment](#preamble-frag) of the metadata stream. | No | `{}` |


Example 62. Variant field class with two options.


```
{
  "type": "variant",
  "selector-field-location": {
    "origin": "event-record-payload",
    "path": ["sel"]
  },
  "options": [
    {
      "selector-field-ranges": [[5, 5]],
      "field-class": {
        "type": "null-terminated-string"
      }
    },
    {
      "selector-field-ranges": [[8, 8]],
      "field-class": {
        "type": "fixed-length-signed-integer",
        "length": 16,
        "byte-order": "little-endian",
        "preferred-display-base": 8
      }
    }
  ]
}
```


Example 63. Variant field class within an [optional field class](#opt-fc), both sharing the same selector field location.


This example shows that an optional field class and a contained variant
field class *MAY* share the same selector field location.


In this example, depending on the value of the selector field:


| 0 | The optional field is *not* enabled. |
|---|---|
| 1 | The optional field is enabled and is a variant field. The variant field is an instance of a [null-terminated string field class](#nt-str-fc) (effective class). |
| 2 | The optional field is enabled and is a variant field. The variant field is an instance of a [variable-length signed integer field class](#vl-int-fc) (effective class). |


```
{
  "type": "optional",
  "selector-field-location": {
    "origin": "event-record-payload",
    "path": ["sel"]
  },
  "selector-field-ranges": [[1, 255]],
  "field-class": {
    "type": "variant",
    "selector-field-location": {
      "origin": "event-record-payload",
      "path": ["sel"]
    },
    "options": [
      {
        "selector-field-ranges": [[1, 1]],
        "field-class": {
          "type": "null-terminated-string"
        }
      },
      {
        "selector-field-ranges": [[2, 2]],
        "field-class": {
          "type": "variable-length-signed-integer",
          "preferred-display-base": 16
        }
      }
    ]
  }
}
```


Example 64. Variant field class with [attributes](#attrs).


```
{
  "type": "variant",
  "selector-field-location": {
    "origin": "event-record-specific-context",
    "path": ["sel"]
  },
  "options": [
    {
      "selector-field-ranges": [[5, 5], [10, 10], [15, 15]],
      "field-class": {
        "type": "static-length-string",
        "length": 20
      }
    },
    {
      "selector-field-ranges": [[0, 4], [6, 9], [11, 14], [16, 127]],
      "field-class": {
        "type": "fixed-length-floating-point-number",
        "length": 32,
        "byte-order": "big-endian"
      }
    }
  ],
  "attributes": {
    "my.tracer": {
      "owner": "Jimmy",
      "id": 199990
    }
  }
}
```


##### 5.3.23.1. Variant field class option


A *variant field class option* contains a possible effective class of a
variant field.


A variant field class option ***O*** also contains the ranges of
integer values (`selector-field-ranges` property) of which the value of
a selector field *MUST* be an element of for the effective class of a
variant field to be the field class of ***O***.


A variant field class option is a JSON object.


| Name | Type | Description | Required? | Default |
|---|---|---|---|---|
| `field-class` | [Field class](#fc) or JSON string | Depending on the type of a value ***V*** of this property: [Field class](#fc) ***V*** is the field class of ***O***. JSON string The effective field class of the previously occurring [field class alias](#fca-frag) named ***V*** is the field class of ***O***. | Yes |  |
| `selector-field-ranges` | [Integer range set](#int-range-set) | Ranges of integers which the value of a selector field *MUST* be an element of for the effective class of an instance of ***F*** to be the field class (`field-class` property) of ***O***. | Yes |  |
| `name` | JSON string | Name of ***O***. This property exists to remain backward compatible with [CTF 1](https://diamon.org/ctf/v1.8.3/): it’s not strictly needed to decode an instance of ***F*** and has no semantic value. | No | ***O*** is unnamed |
| `attributes` | [Attributes](#attrs) | Attributes of ***O***. | No | `{}` |
| `extensions` | [Extensions](#ext) | Extensions of ***O***. Any extension which exists under this property *MUST* also be declared in the [preamble fragment](#preamble-frag) of the metadata stream. | No | `{}` |


Example 65. Unnamed [null-terminated string field class](#nt-str-fc) option.


```
{
  "field-class": {
    "type": "null-terminated-string"
  },
  "selector-field-ranges": [[3, 9]]
}
```


Example 66. [Variable-length signed integer field class](#vl-int-fc) option named `juice` with [attributes](#attrs).


```
{
  "name": "juice",
  "field-class": {
    "type": "variable-length-signed-integer",
    "preferred-display-base": 16
  },
  "selector-field-ranges": [[-4, 4], [9, 9], [100, 200]],
  "attributes": {
    "my.tracer": {
      "uuid": [
        243, 97, 0, 184, 236, 54, 72, 97,
        141, 107, 169, 214, 171, 137, 115, 201
      ],
      "is-did": true
    }
  }
}
```


### 5.4. Preamble fragment


A *preamble fragment* indicates:


- The CTF 2 major version (2).

CTF 2 doesn’t have a minor version: a [producer](#producer-def) can use
[attributes](#attributes) and [extensions](#ext) to add features to, or
change features of, the format which this document specifies.

- *OPTIONAL*: The UUID of the whole [metadata
stream](#metadata-stream-overview).

The purpose of such a UUID is to associate data stream [packets](#pkt)
to a specific metadata stream through the
[`metadata-stream-uuid` role](#metadata-stream-uuid-role) of a
[packet header](#pkt-header) [static-length BLOB](#sl-blob-fc) field.

- *OPTIONAL*: [Extension](#ext) declarations.

An extension declaration is an initial extension of which the purpose is
to declare that it’s *enabled* within the [metadata
stream](#metadata-stream).


Because an extension *MAY* alter the CTF 2 format itself, and because a
preamble fragment is always the first metadata stream fragment, those
extension declarations make it possible for a [consumer](#consumer-def)
to gracefully decline the [data streams](#ds) of the trace if it doesn’t
support *any* declared extension.


The first fragment of a metadata stream *MUST* be a preamble fragment.


| Name | Type | Description | Required? | Default |
|---|---|---|---|---|
| `type` | JSON string | Type of ***F***. The value of this property *MUST* be `"preamble"`. | Yes |  |
| `version` | JSON integer | CTF 2 major version. The value of this property *MUST* be `2`. | Yes |  |
| `uuid` | JSON array of 16 JSON integers | [UUID](https://en.wikipedia.org/wiki/Universally_unique_identifier) of the whole [metadata stream](#metadata-stream-overview). The purpose of this UUID is to associate data stream [packets](#pkt) to a specific metadata stream through the [`metadata-stream-uuid` role](#metadata-stream-uuid-role) of a [packet header](#pkt-header) [static-length BLOB](#sl-blob-fc) field. The 16 JSON integers of this JSON array are the numeric values of the 16 bytes of the UUID. | No | The metadata stream has no UUID |
| `attributes` | [Attributes](#attrs) | Attributes of ***F***. | No | `{}` |
| `extensions` | [Extensions](#ext) | Extension declarations of ***F***. The name of each property is a [namespace](#ns-def) and its value is a [namespaced extensions object](#ns-exts-obj). Within a [namespaced extensions object](#ns-exts-obj), an extension named ***N*** is *declared* when it exists as a property named ***N***, whatever the value of the property. | No | `{}` |


Example 67. Minimal preamble fragment.


```
{
  "type": "preamble",
  "version": 2
}
```


Example 68. Preamble fragment with [extension](#ext) declarations.


The following preamble fragment declares the `piano` and `ramen`
extensions under the `my.tracer` namespace.


```
{
  "type": "preamble",
  "version": 2,
  "extensions": {
    "my.tracer": {
      "piano": {
        "keys": 88,
        "temperament": "equal"
      },
      "ramen": null
    }
  }
}
```


Example 69. Preamble fragment with a [metadata stream UUID](#metadata-stream-uuid).


```
{
  "type": "preamble",
  "version": 2,
  "uuid": [
    229, 62, 10, 184, 80, 161, 79, 10,
    183, 16, 181, 240, 187, 169, 196, 172
  ]
}
```


### 5.5. Field class alias fragment


A *field class alias* assigns a name to a [field class](#fc).


The following JSON object properties *MAY* refer to the *effective field
class* of a previously occurring field class alias using its name (JSON
string):


Field class alias fragment

`field-class`


[Trace class fragment](#tc-frag)

`packet-header-field-class`


[Data stream class fragment](#dsc-frag)


- `packet-context-field-class`

- `event-record-header-field-class`

- `event-record-common-context-field-class`


[Event record class fragment](#erc-frag)


- `specific-context-field-class`

- `payload-field-class`


[Structure field member class](#struct-member-cls)

`field-class`


[Abstract array field class](#array-fc)

`element-field-class`


[Optional field class](#opt-fc)

`field-class`


[Variant field class option](#var-fc-opt)

`field-class`


| Name | Type | Description | Required? | Default |
|---|---|---|---|---|
| `type` | JSON string | Type of ***F***. The value of this property *MUST* be `"field-class-alias"`. | Yes |  |
| `name` | JSON string | Name of ***F***. | Yes |  |
| `field-class` | [Field class](#fc) or JSON string | Depending on the type of a value ***V*** of this property: [Field class](#fc) ***V*** is the effective field class of ***F***. JSON string The effective field class of the previously occurring field class alias named ***V*** is the effective field class of ***F***. | Yes |  |
| `attributes` | [Attributes](#attrs) | Attributes of ***F***. | No | `{}` |
| `extensions` | [Extensions](#ext) | Extensions of ***F***. Any extension which exists under this property *MUST* also be declared in the [preamble fragment](#preamble-frag) of the metadata stream. | No | `{}` |


Within a metadata stream, two given field class alias fragments
*MUST NOT* share the same `name` property value.


Example 70. Field class alias fragment assigning the name `u8` to an 8-bit [fixed-length unsigned integer field class](#fl-int-fc).


```
{
  "type": "field-class-alias",
  "name": "u8",
  "field-class": {
    "type": "fixed-length-unsigned-integer",
    "length": 8,
    "byte-order": "little-endian"
  }
}
```


Example 71. Field class alias fragment assigning the name `uint8` to an aliased field class named `u8`.


```
{
  "type": "field-class-alias",
  "name": "uint8",
  "field-class": "u8"
}
```


### 5.6. Trace class fragment


A *trace class* describes [traces](#trace).


Within a metadata stream, if a trace class fragment ***T***
exists, then ***T*** *MUST* occur before any [data
stream class fragment](#dsc-frag).


| Name | Type | Description | Required? | Default |
|---|---|---|---|---|
| `type` | JSON string | Type of ***F***. The value of this property *MUST* be `"trace-class"`. | Yes |  |
| `namespace` | JSON string | [Namespace](#ns-def) of ***F***. | No | An instance of ***F*** has no namespace |
| `name` | JSON string | Name of ***F***. | No | An instance of ***F*** has no name |
| `uid` | JSON string | Unique ID (any string) of ***F*** within some scope identified by the `namespace` (if any) and `name` (if any) properties. | No | An instance of ***F*** has no unique ID |
| `environment` | JSON object, where each property is: Name Environment entry name. Value Environment entry value (JSON integer or JSON string). | Environment of an instance of ***F***. This property exists to remain backward compatible with [CTF 1](https://diamon.org/ctf/v1.8.3/): this document doesn’t specify what an environment entry exactly is. | No | `{}` |
| `packet-header-field-class` | [Structure field class](#struct-fc) or JSON string | Depending on the type of a value ***V*** of this property: [Structure field class](#struct-fc) ***V*** is the class of all the [packet header fields](#pkt-header) of an instance of ***F***. JSON string The effective field class of the previously occurring *structure* [field class alias](#fca-frag) named ***V*** is the class of all the packet header fields of an instance of ***F***. Any field class contained in the packet header field class *MUST* satisfy *at least one of*: Have at least one valid [role](#pkt-header-roles). Be a [structure field class](#struct-fc). Be an [optional field class](#opt-fc). Be a [variant field class](#var-fc). Be the class of a field which is the selector field of an optional or variant field (anywhere within the same [packet](#pkt)). | No | ***F*** has no packet header field class |
| `attributes` | [Attributes](#attrs) | Attributes of ***F***. | No | `{}` |
| `extensions` | [Extensions](#ext) | Extensions of ***F***. Any extension which exists under this property *MUST* also be declared in the [preamble fragment](#preamble-frag) of the metadata stream. | No | `{}` |


#### 5.6.1. Roles


 If the `packet-header-field-class` property of a
trace class fragment exists, then the [field classes](#fc) of its
[member classes](#struct-member-cls) *MAY* have the following
[roles](#roles):


| Name | Description | Field class (***F***) constraints | Other constraints |
|---|---|---|---|
| `data-stream-class-id` | Current data stream class ID. The purpose of a data stream class ID field is to set the current ID of the class of the data stream of the current packet. | [Fixed-length unsigned integer field class](#fl-int-fc) or [variable-length unsigned integer field class](#vl-int-fc). |  |
| `data-stream-id` | Current data stream ID. The purpose of a data stream ID field is to set the current ID of the data stream of the current packet. Combined with the ID of its class, such a field makes it possible to uniquely identify a data stream within a [trace](#trace). | [Fixed-length unsigned integer field class](#fl-int-fc) or [variable-length unsigned integer field class](#vl-int-fc). |  |
| `metadata-stream-uuid` | [Metadata stream UUID](#metadata-stream-uuid). | [Static-length BLOB field class](#sl-blob-fc) with the following property value: `length` | `16` |


The [`uuid` property of the preamble fragment](#metadata-stream-uuid)
*MUST* exist.


The 16 bytes of an instance of ***F*** *MUST* be equal to the
16 JSON integers of the `uuid` property of the preamble fragment.


`packet-magic-number`


[Packet](#pkt) magic number.


The purpose of a packet magic number field is to confirm the beginning
of a CTF 2 packet.


[Fixed-length unsigned integer field class](#fl-int-fc) with the following property value:


| `length` | `32` |
|---|---|


An instance of ***F*** *MUST* be the *first* member of the packet
header structure field.


The value of an instance of ***F*** *MUST* be 0xc1fc1fc1
(3,254,525,889).


Example 72. Metadata stream fragments.


Preamble fragment.


```
{
  "type": "preamble",
  "version": 2,
  "uuid": [
    30, 201, 100, 148, 228, 2, 69, 70,
    147, 219, 233, 34, 43, 238, 108, 199
  ]
}
```


Trace class fragment.


```
{
  "type": "trace-class",
  "packet-header-field-class": {
    "type": "structure",
    "member-classes": [
      {
        "name": "the magic!",
        "field-class": {
          "type": "fixed-length-unsigned-integer",
          "length": 32,
          "byte-order": "little-endian",
          "preferred-display-base": 16,
          "roles": ["packet-magic-number"]
        }
      },
      {
        "name": "the UUID",
        "field-class": {
          "type": "static-length-blob",
          "length": 16,
          "roles": ["metadata-stream-uuid"]
        }
      },
      {
        "name": "my data stream class ID",
        "field-class": {
          "type": "fixed-length-unsigned-integer",
          "length": 8,
          "byte-order": "little-endian",
          "roles": ["data-stream-class-id"]
        }
      },
      {
        "name": "my data stream ID",
        "field-class": {
          "type": "variable-length-unsigned-integer",
          "roles": ["data-stream-id"]
        }
      }
    ]
  }
}
```


### 5.7. Clock class fragment


A *clock class* describes *clocks*.


A [data stream](#ds) *MAY* have a [default clock](#def-clk).


Within a metadata stream, a clock class fragment *MUST* occur before any
[data stream class fragment](#dsc-frag) which refers to it by internal
ID with its `default-clock-class-id` property.


| Name | Type | Description | Required? | Default |
|---|---|---|---|---|
| `type` | JSON string | Type of ***F***. The value of this property *MUST* be `"clock-class"`. | Yes |  |
| `id` | JSON string | Internal ID of ***F***. The sole purpose of this property is to indicate the default clock class of a [data stream class](#dsc-frag) through its `default-clock-class-id` property. | Yes |  |
| `namespace` | JSON string | [Namespace](#ns-def) of ***F***. | No | An instance of ***F*** has no namespace |
| `name` | JSON string | Name of ***F***. | No | An instance of ***F*** has no name |
| `uid` | JSON string | Unique ID (any string) of ***F*** within some scope identified by the `namespace` (if any) and `name` (if any) properties. | No | An instance of ***F*** has no unique ID |
| `frequency` | JSON integer | Frequency of an instance of ***F*** (Hz). The value of this property *MUST* be greater than zero. | Yes |  |
| `origin` | JSON string or [clock origin](#clock-origin) | Origin of an instance of ***F***. The origin of a clock is what makes it possible to establish a correlation with other clocks having different [identities](#clk-id), possibly amongst different [traces](#trace), while also considering the `precision` and `accuracy` properties. The value of this property *MUST* be one of: `"unix-epoch"` The origin of an instance of ***F*** is the [Unix epoch](https://en.wikipedia.org/wiki/Unix_time). A clock having the Unix epoch as its origin has a correlation with other clocks sharing this origin. A [clock origin](#clock-origin) object ***CO*** The origin of an instance of ***F*** is described by ***CO***. A clock having an origin described by ***CO*** has a correlation with other clocks having an origin described by a clock origin which is *identical* to ***CO***. | No | An instance of ***F*** has no known origin: it has no correlation with any other clock having a different [identity](#clk-id) |
| `offset-from-origin` | [Clock offset](#clock-offset) | Offset of an instance of ***F*** relative to its origin (see the `origin` property). Let: ***H*** be the value of the `frequency` property of ***F***. ***O*** be the value of this property. ***S*** be the value of the `seconds` property of ***O***. ***C*** be the value of the `cycles` property of ***O***. Then the effective offset of an instance of ***F*** from its origin, in clock cycles, is ***S*** × ***H*** + ***C***. | No | `{}` |
| `precision` | JSON integer | Precision of an instance of ***F*** (clock cycles). This property describes the random errors of an instance of ***F***. The value of this property *MUST* be greater than or equal to zero. | No | The precision of an instance of ***F*** is unknown |
| `accuracy` | JSON integer | Accuracy of an instance of ***F*** (clock cycles). This property describes the systematic errors of an instance of ***F***. The value of this property *MUST* be greater than or equal to zero. | No | The accuracy of an instance of ***F*** is unknown |
| `description` | JSON string | Textual description of ***F***. This property exists to remain backward compatible with [CTF 1](https://diamon.org/ctf/v1.8.3/): it’s not strictly needed to decode [data streams](#ds) and has no semantic value. | No | ***F*** has no textual description |
| `attributes` | [Attributes](#attrs) | Attributes of ***F***. | No | `{}` |
| `extensions` | [Extensions](#ext) | Extensions of ***F***. Any extension which exists under this property *MUST* also be declared in the [preamble fragment](#preamble-frag) of the metadata stream. | No | `{}` |


 Two clock classes which satisfy *all* the following
conditions are said to have the same *identity*:


- Both share the same `namespace` property value, or both don’t have any
`namespace` property.

- Both have a `name` property and share the same value.

- Both have a `uid` property and share the same value.


Two clock classes having the same identity, within the same
[metadata stream](#metadata-stream-overview) or in two different
metadata streams, *MUST* also share:


- The same `frequency` property value.

- The same `precision` property value, if any.

- The same `accuracy` property value, if any.

- The same `origin` property value, if any.


The `precision` and `accuracy` properties of a clock class respectively
describe the [random
and systematic errors](https://en.wikipedia.org/wiki/Accuracy_and_precision) of its instances. Moreover:


- For a clock class ***CC***, let:


The value of the `precision` property of ***CC*** be ***P***.

- The value of the `accuracy` property of ***CC*** be ***A***.

- ***V*** be the value of an instance of ***CC***.


Then the range of possible *true* values of the instance
is [***V*** − ***P*** − ***A***, ***V*** + ***P*** + ***A***].


- Let two clock classes ***CCA*** and ***CCB*** have the
same [identity](#clk-id): when computing value differences between
instances of ***CCA*** and ***CCB***, the value of their
`precision` property is relevant, but the value of their `accuracy`
property isn’t.

- Let two clock classes ***CCA*** and ***CCB*** not have
the same identity, but share the same `origin` property value: when
computing value differences between instances of ***CCA***
and ***CCB***, both the values of their `precision` and
`accuracy` properties are relevant.


Within a metadata stream, two given clock class fragments *MUST NOT*
share the same `id` property value.


Example 73. Minimal clock class fragment with 1-GHz instances having no known origin.


```
{
  "type": "clock-class",
  "id": "my clock class",
  "frequency": 1000000000
}
```


Example 74. Minimal clock class fragment with instances having the Unix epoch as their origin.


```
{
  "type": "clock-class",
  "id": "my clock class",
  "frequency": 1000000000,
  "origin": "unix-epoch"
}
```


Example 75. Clock class fragment with a specific [identity](#clk-id).


```
{
  "type": "clock-class",
  "id": "my clock class",
  "namespace": "some-tracer",
  "name": "hot",
  "uid": "wheels.28",
  "frequency": 80000000
}
```


Example 76. Clock class fragment with instances having a custom origin.


```
{
  "type": "clock-class",
  "id": "my clock class",
  "frequency": 1000000000,
  "origin": {
    "name": "my origin",
    "uid": "60:57:18:a3:42:29"
  }
}
```


Example 77. Clock class fragment with instances having a specific offset from origin.


```
{
  "type": "clock-class",
  "id": "my clock class",
  "frequency": 1000000000,
  "offset-from-origin": {
    "seconds": 1605112699,
    "cycles": 2878388
  }
}
```


Example 78. Clock class fragment with instances having a known precision of 100 cycles.


```
{
  "type": "clock-class",
  "id": "my clock class",
  "frequency": 8000000,
  "precision": 100
}
```


Example 79. Clock class fragment with instances having a known accuracy of 1500 cycles.


```
{
  "type": "clock-class",
  "id": "my clock class",
  "frequency": 16000000,
  "accuracy": 1500
}
```


Example 80. Clock class fragment with [attributes](#attrs).


```
{
  "type": "clock-class",
  "id": "my clock class",
  "frequency": 32000000,
  "attributes": {
    "my.tracer": {
      "sys-name": "SOC23",
      "bus": {
        "name": "LMB5",
        "index": 5
      },
      "propagation-delay-ps": 177
    }
  }
}
```


#### 5.7.1. Clock origin


A *clock origin* specifies the origin of the instances of a
[clock class](#cc-frag).


A clock origin is a JSON object.


| Name | Type | Description | Required? | Default |
|---|---|---|---|---|
| `namespace` | JSON string | [Namespace](#ns-def) of ***CO***. | No | ***CO*** has no namespace |
| `name` | JSON string | Name of ***CO***. | Yes |  |
| `uid` | JSON string | Unique ID (any string) of ***CO*** within some scope identified by the `namespace` (if any) and `name` properties. | Yes |  |


Considering two clock class fragments ***CCA***
and ***CCB***, which don’t need to be part of the same
[metadata stream](#metadata-stream-overview), both having an `origin`
property set to some clock origin object: an instance
of ***CCA*** has a correlation with an instance
of ***CCB*** if and only if the clock origin objects
of ***CCA*** and ***CCB*** are identical.


Example 81. Clock origin without a namespace.


```
{
  "name": "my-origin",
  "uid": "42"
}
```


Example 82. Clock origin with a namespace.


```
{
  "namespace": "some-tracer",
  "name": "the-origin",
  "uid": "7e41f662-27b6-45d5-8d9b-0504daa7b04a"
}
```


#### 5.7.2. Clock offset


A *clock offset* contains the offset of the instances of a
[clock class](#cc-frag) relative to their origin (see the `origin`
property).


A clock offset is a JSON object.


| Name | Type | Description | Required? | Default |
|---|---|---|---|---|
| `seconds` | JSON integer | Offset, in seconds, of an instance of ***F*** relative to its origin. | No | `0` |
| `cycles` | JSON integer | Offset, in cycles, of an instance of ***F*** relative to its origin. The value of this property *MUST* be greater than or equal to zero. The value of this property *MUST* be less than the value of the `frequency` property of ***F***. | No | `0` |


Example 83. Minimal clock offset.


```
{}
```


Example 84. Clock offset with seconds and cycles.


```
{
  "seconds": 1605112699,
  "cycles": 2878388
}
```


Example 85. Clock offset with seconds only.


```
{
  "seconds": 1605111293
}
```


Example 86. Negative clock offset.


This example shows that a clock offset *MAY* be negative, that is,
*before* the origin of the clock.


```
{
  "seconds": -18003,
  "cycles": 11928547
}
```


### 5.8. Data stream class fragment


A *data stream class* describes [data streams](#ds).


Within a metadata stream, a data stream class fragment ***F*** *MUST*
occur before any [event record class fragment](#erc-frag) of
which ***F*** is the parent.


| Name | Type | Description | Required? | Default |
|---|---|---|---|---|
| `type` | JSON string | Type of ***F***. The value of this property *MUST* be `"data-stream-class"`. | Yes |  |
| `id` | JSON integer | Numeric ID of ***F***. The value of this property *MUST* be greater than or equal to zero. | No | `0` |
| `namespace` | JSON string | [Namespace](#ns-def) of ***F***. | No | An instance of ***F*** has no namespace |
| `name` | JSON string | Name of ***F***. | No | An instance of ***F*** has no name |
| `uid` | JSON string | Unique ID (any string) of ***F*** within some scope identified by the `namespace` (if any) and `name` (if any) properties. | No | An instance of ***F*** has no unique ID |
| `default-clock-class-id` | JSON string | Internal ID of the [class](#cc-frag) of the [default clock](#def-clk) of an instance of ***F***. Within the metadata stream containing ***F***, the [clock class fragment](#cc-frag) which has the value of this property as its `id` property *MUST* occur before ***F***. | No | An instance of ***F*** has no default clock |
| `packet-context-field-class` | [Structure field class](#struct-fc) or JSON string | Depending on the type of a value ***V*** of this property: [Structure field class](#struct-fc) ***V*** is the class of all the [packet context fields](#pkt-ctx) of an instance of ***F***. JSON string The effective field class of the previously occurring *structure* [field class alias](#fca-frag) named ***V*** is the class of all the packet context fields of an instance of ***F***. | No | ***F*** has no packet context field class |
| `event-record-header-field-class` | [Structure field class](#struct-fc) or JSON string | Depending on the type of a value ***V*** of this property: [Structure field class](#struct-fc) ***V*** is the class of all the [event record header fields](#er-header) of an instance of ***F***. JSON string The effective field class of the previously occurring *structure* [field class alias](#fca-frag) named ***V*** is the class of all the event record header fields of an instance of ***F***. Any field class contained in the event record header field class *MUST* satisfy *at least one of*: Have at least one valid [role](#er-header-roles). Be a [structure field class](#struct-fc). Be an [optional field class](#opt-fc). Be a [variant field class](#var-fc). Be the class of a field which is the selector field of an optional or variant field (anywhere within the same [event record](#er)). | No | ***F*** has no event record header field class |
| `event-record-common-context-field-class` | [Structure field class](#struct-fc) or JSON string | Depending on the type of a value ***V*** of this property: [Structure field class](#struct-fc) ***V*** is the class of all the [event record common context fields](#er-common-ctx) of an instance of ***F***. JSON string The effective field class of the previously occurring *structure* [field class alias](#fca-frag) named ***V*** is the class of all the event record common context fields of an instance of ***F***. | No | ***F*** has no event record common context field class |
| `attributes` | [Attributes](#attrs) | Attributes of ***F***. | No | `{}` |
| `extensions` | [Extensions](#ext) | Extensions of ***F***. Any extension which exists under this property *MUST* also be declared in the [preamble fragment](#preamble-frag) of the metadata stream. | No | `{}` |


Within a metadata stream, two given data stream class fragments
*MUST NOT* share the same `id` property value.


#### 5.8.1. Roles


 If the `packet-context-field-class` property of a data
stream class fragment exists, then the [field classes](#fc) of its
[member classes](#struct-member-cls) *MAY* have the following
[roles](#roles):


| Name | Description | Field class (***F***) constraints | Other constraints |
|---|---|---|---|
| `default-clock-timestamp` | Current timestamp of the [default clock](#def-clk) of the data stream when the packet begins. | [Fixed-length unsigned integer field class](#fl-int-fc) or [variable-length unsigned integer field class](#vl-int-fc). | The data stream class has a `default-clock-class-id` property. |
| `discarded-event-record-counter-snapshot` | Current snapshot of the [discarded event record counter](#disc-er-counter) of the data stream when the packet ends. | [Fixed-length unsigned integer field class](#fl-int-fc) or [variable-length unsigned integer field class](#vl-int-fc). |  |
| `packet-content-length` | Current content length (bits) of the packet. | [Fixed-length unsigned integer field class](#fl-int-fc) or [variable-length unsigned integer field class](#vl-int-fc). |  |
| `packet-end-default-clock-timestamp` | Current timestamp of the default clock of the data stream when the packet ends. | [Fixed-length unsigned integer field class](#fl-int-fc) or [variable-length unsigned integer field class](#vl-int-fc). | The data stream class has a `default-clock-class-id` property. |
| `packet-sequence-number` | Current sequence number of the packet within its data stream. | [Fixed-length unsigned integer field class](#fl-int-fc) or [variable-length unsigned integer field class](#vl-int-fc). |  |
| `packet-total-length` | Current total length (bits) of the [packet](#pkt). | [Fixed-length unsigned integer field class](#fl-int-fc) or [variable-length unsigned integer field class](#vl-int-fc). |  |


 If the `event-record-header-field-class` property of
a data stream class fragment exists, then the [field classes](#fc) of
its [member classes](#struct-member-cls) *MAY* have the following
[roles](#roles):


| Name | Description | Field class (***F***) constraints | Other constraints |
|---|---|---|---|
| `default-clock-timestamp` | Current timestamp of the [default clock](#def-clk) of the data stream when the event record occurs. | [Fixed-length unsigned integer field class](#fl-int-fc) or [variable-length unsigned integer field class](#vl-int-fc). | The data stream class has a `default-clock-class-id` property. |
| `event-record-class-id` | Current event record class ID. The purpose of a field having this role is to set the current ID of the class of the event record within its parent [data stream class](#dsc-frag). | [Fixed-length unsigned integer field class](#fl-int-fc) or [variable-length unsigned integer field class](#vl-int-fc). |  |


### 5.9. Event record class fragment


An *event record class* describes [event records](#er).


The [data stream class fragment](#dsc-frag) of which the value of the
`id` property matches the value of the `data-stream-class-id` property
of an event record class fragment ***F*** is considered the
*parent* of ***F***.


| Name | Type | Description | Required? | Default |
|---|---|---|---|---|
| `type` | JSON string | Type of ***F***. The value of this property *MUST* be `"event-record-class"`. | Yes |  |
| `id` | JSON integer | Numeric ID of ***F*** within ***P***. The value of this property *MUST* be greater than or equal to zero. | No | `0` |
| `data-stream-class-id` | JSON integer | Numeric ID of ***P***. The value of this property *MUST* be greater than or equal to zero. Within the metadata stream, ***P*** *MUST* occur before ***F***. | No | `0` |
| `namespace` | JSON string | [Namespace](#ns-def) of ***F***. | No | An instance of ***F*** has no namespace |
| `name` | JSON string | Name of ***F***. | No | An instance of ***F*** has no name |
| `uid` | JSON string | Unique ID (any string) of ***F*** within some scope identified by the `namespace` (if any) and `name` (if any) properties. | No | An instance of ***F*** has no unique ID |
| `specific-context-field-class` | [Structure field class](#struct-fc) or JSON string | Depending on the type of a value ***V*** of this property: [Structure field class](#struct-fc) ***V*** is the class of the [specific context field](#er-spec-ctx) of an instance of ***F***. JSON string The effective field class of the previously occurring *structure* [field class alias](#fca-frag) named ***V*** is the class of the specific context field of an instance of ***F***. | No | ***F*** has no event record specific context field class |
| `payload-field-class` | [Structure field class](#struct-fc) or JSON string | Depending on the type of a value ***V*** of this property: [Structure field class](#struct-fc) ***V*** is the class of the [payload field](#er-payload) of an instance of ***F***. JSON string The effective field class of the previously occurring *structure* [field class alias](#fca-frag) named ***V*** is the class of the payload field of an instance of ***F***. | No | ***F*** has no event record payload field class |
| `attributes` | [Attributes](#attrs) | Attributes of ***F***. | No | `{}` |
| `extensions` | [Extensions](#ext) | Extensions of ***F***. Any extension which exists under this property *MUST* also be declared in the [preamble fragment](#preamble-frag) of the metadata stream. | No | `{}` |


Within a metadata stream, two given event record class fragments
*MUST NOT* share both the same `id` *and* `data-stream-class-id`
property values.


## 6. Data stream decoding procedure


This section shows how to, procedurally, *decode* a CTF 2 [data
stream](#ds).


Decoding a data stream is the responsibility of a
[consumer](#consumer-def).


This document does not detail the method for encoding a data stream
since encoding offers significantly more flexibility than decoding.
However, one can infer the encoding process based on the outlined
decoding procedure.


To decode a data stream ***S***:


- While there’s remaining data in ***S***:


[Decode one packet](#pkt-dec).


### 6.1. Packet decoding procedure


A [consumer](#consumer-def) needs to keep a *packet decoding state*
while decoding a [packet](#pkt). A packet decoding state comprises the
following *variables*:


| Name | Type | Description | Initial value |
|---|---|---|---|
| ***X*** | Unsigned integer | Current decoding offset/position (bits) from the beginning of ***P***. | 0 |
| ***DEF_CLK_VAL*** | Unsigned integer | If the [class](#dsc-frag) of ***S*** has a `default-clock-class-id` property: current value (clock cycles) of the [default clock](#def-clk) of ***S***. | 0 |
| ***DISC_ER_SNAP*** | *OPTIONAL* unsigned integer | Current snapshot of the [discarded event record counter](#disc-er-counter) of ***S*** at the end of ***P***. | None |
| ***DSC*** | *OPTIONAL* [data stream class](#dsc-frag) | Current class of ***S***. | None |
| ***DSC_ID*** | Unsigned integer | Current ID of the [class](#dsc-frag) of ***S***. | 0 |
| ***DS_ID*** | *OPTIONAL* unsigned integer | Current ID of ***S***. | None |
| ***LAST_BYTE_ORDER*** | *OPTIONAL* string | Byte order of the last [decoded fixed-length bit array field](#fl-ba-field-dec). | None |
| ***PKT_CONTENT_LEN*** | Unsigned integer | Current content length (bits) of ***P***. | ∞ |
| ***PKT_END_DEF_CLK_VAL*** | *OPTIONAL* unsigned integer | If the [class](#dsc-frag) of ***S*** has a `default-clock-class-id` property: current value (clock cycles) of the [default clock](#def-clk) of ***S*** at the end of ***P***. | None |
| ***PKT_SEQ_NUM*** | *OPTIONAL* unsigned integer | Current sequence number of ***P***. | None |
| ***PKT_TOTAL_LEN*** | Unsigned integer | Current total length (bits) of ***P***. | ∞ |


To decode a packet ***P*** within a data stream ***S***:


- If the `packet-header-field-class` property of the [trace
class fragment](#tc-frag) of the metadata stream exists, then
[decode](#struct-field-dec) the [header field](#pkt-header)
of ***P*** using the value of this property.

During the packet header field decoding procedure, after having decoded
a field ***F*** as ***V***, with ***F*** having the
class ***C*** with a `roles` property:


If ***C*** has the role `data-stream-class-id`, then
set ***DSC_ID*** to ***V***.

- If ***C*** has the role `data-stream-id`, then
set ***DS_ID*** to ***V***.

- If ***C*** has the role `packet-magic-number`, then validate
that ***V*** is 0xc1fc1fc1 (3,254,525,889).

A [consumer](#consumer-def) *SHOULD* report an invalid packet magic
number as an error.

- If ***C*** has the role `metadata-stream-uuid`, then validate
that ***V*** matches the [`uuid` property
of the preamble fragment](#metadata-stream-uuid) of the [metadata
stream](#metadata-stream-overview) of the [trace](#trace) of ***S***.

A consumer *SHOULD* report a metadata stream UUID mismatch as an error.


After having decoded the whole packet header field,
if ***DS_ID*** is set, then it’s the ID of ***S*** within
its [class](#dsc-frag). In other words, two data streams *MAY* have the
same ID if they’re instances of different data stream classes.


- Set ***DSC*** to the [data stream class](#dsc-frag)
having ***DSC_ID*** as the value of its `id` property.

If no data stream class has the ID ***DSC_ID***, then report an
error and abort the data stream decoding process.

- If the `packet-context-field-class` property of ***DSC***
exists, then [decode](#struct-field-dec) the [context field](#pkt-ctx)
of ***P*** using the value of this property.

During the packet context field decoding procedure, after having decoded
a field ***F*** as ***V***, with ***F*** having the
class ***C*** with a `roles` property:


If ***C*** has the role `default-clock-timestamp`, then
[update DEF_CLK_VAL](#clk-val-update) from ***F***.

- If ***C*** has the role `discarded-event-record-counter-snapshot`,
then set ***DISC_ER_SNAP*** to ***V***.

- If ***C*** has the role `packet-content-length`, then
set ***PKT_CONTENT_LEN*** to ***V***.

- If ***C*** has the role `packet-end-default-clock-timestamp`, then
set ***PKT_END_DEF_CLK_VAL*** to ***V***.

- If ***C*** has the role `packet-sequence-number`, then
set ***PKT_SEQ_NUM*** to ***V***.

- If ***C*** has the role `packet-total-length`, then
set ***PKT_TOTAL_LEN*** to ***V***.


After having decoded the whole packet context field:


- If set, ***DEF_CLK_VAL*** is the value of the [default
clock](#def-clk) of ***S*** at the *beginning* of ***P***.

- If set, ***PKT_END_DEF_CLK_VAL*** is the value of the
default clock of ***S*** at the *end* of ***P***.

- If both ***DEF_CLK_VAL*** and ***PKT_END_DEF_CLK_VAL*** are set,
and if ***DEF_CLK_VAL*** > ***PKT_END_DEF_CLK_VAL***
(packet beginning timestamp is greater than packet end timestamp),
then a [consumer](#consumer-def) *SHOULD* report an error.

- If ***PKT_TOTAL_LEN*** is ∞ and ***PKT_CONTENT_LEN*** is
*not* ∞, then set ***PKT_TOTAL_LEN***
to ***PKT_CONTENT_LEN***.

- If ***PKT_CONTENT_LEN*** is ∞ and ***PKT_TOTAL_LEN*** is
*not* ∞, then set ***PKT_CONTENT_LEN***
to ***PKT_TOTAL_LEN***.

- If both ***PKT_TOTAL_LEN*** and ***PKT_CONTENT_LEN***
are *not* ∞, and if
***PKT_CONTENT_LEN*** > ***PKT_TOTAL_LEN***, then
report an error and abort the data stream decoding process.

- If set, ***DISC_ER_SNAP*** is a snapshot of the
[discarded event record counter](#disc-er-counter) of ***S*** at
the end of ***P***.

- If set, ***PKT_SEQ_NUM*** is the sequence number of ***P***.


- While ***X*** < ***PKT_CONTENT_LEN***
and there’s remaining data in ***S***:


[Decode an event record](#er-dec).


- If ***PKT_TOTAL_LEN*** and ***PKT_CONTENT_LEN*** both are
*not* ∞, then set ***X***
to ***PKT_TOTAL_LEN***, effectively skipping end-of-packet
padding.


### 6.2. Event record decoding procedure


A [consumer](#consumer-def) needs to keep an *event record decoding
state* while decoding an [event record](#er). An event record decoding
state comprises the following *variables*:


| Name | Type | Description | Initial value |
|---|---|---|---|
| ***ERC_ID*** | Unsigned integer | Current ID of the [class](#erc-frag) of ***E*** of which the parent is the [class](#dsc-frag) of ***S***. | 0 |
| ***ERC*** | *OPTIONAL* [event record class](#erc-frag) | Current class of ***E***. | None |


To decode an event record ***E*** within a data stream ***S***:


- If the `event-record-header-field-class` property of ***DSC***
exists, then [decode](#struct-field-dec) the [header
field](#er-header) of ***E*** using the value of this property.

During the event record header field decoding procedure, after having
decoded a field ***F*** as ***V***, with ***F*** having
the class ***C*** with a `roles` property:


If ***C*** has the role `event-record-class-id`, then
set ***ERC_ID*** to ***V***.

- If ***C*** has the role `default-clock-timestamp`, then
[update DEF_CLK_VAL](#clk-val-update) from ***F***.


After having decoded the whole event record header field,
***DEF_CLK_VAL*** is the value of the [default clock](#def-clk)
of ***S*** when ***E*** occurs.


- Set ***ERC*** to the [event record class](#erc-frag) having:


***DSC_ID*** as the value of its `data-stream-class-id` property.

- ***ERC_ID*** as the value of its `id` property.


If no event record class has the ID ***ERC_ID*** within a data
stream class having the ID ***DSC_ID***, then report an error and
abort the data stream decoding process.


- If the `event-record-common-context-field-class` property
of ***DSC*** exists, then [decode](#struct-field-dec) the
[common context field](#er-common-ctx) of ***E*** using the
value of this property.

- If the `specific-context-field-class` property of ***ERC***
exists, then [decode](#struct-field-dec) the
[specific context field](#er-spec-ctx) of ***E***
using the value of this property.

- If the `payload-field-class` property of ***ERC*** exists, then
[decode](#struct-field-dec) the [payload field](#er-payload)
of ***E*** using the value of this property.


### 6.3. Clock value update procedure


To update ***DEF_CLK_VAL*** from an unsigned integer
field ***F*** having the unsigned integer value ***V*** and
the [class](#fc) ***C***:


- Let ***L*** be an unsigned integer initialized to,
depending on the `type` property of ***C***:


[`"fixed-length-unsigned-integer"`](#fl-int-fc)

The value of the `length` property of ***C***.

[`"variable-length-unsigned-integer"`](#vl-int-fc)

***S*** × 7, where ***S*** is the number of
[bytes](#byte-def) which ***F*** occupies
within the [data stream](#ds).

- Let ***MASK*** be an unsigned integer initialized to
2***L*** − 1.

- Let ***H*** be an unsigned integer initialized to
***DEF_CLK_VAL*** & ~***MASK***,
where “&” is the bitwise *AND* operator and
“~” is the bitwise *NOT* operator.

- Let ***CUR*** be an unsigned integer initialized to
***DEF_CLK_VAL*** & ***MASK***, where “&” is the
bitwise *AND* operator.

- Set ***DEF_CLK_VAL*** to:


If ***V*** ≥ ***CUR***

***H*** + ***V***

Else

***H*** + ***MASK*** + 1 + ***V***


### 6.4. Field decoding procedure


The [class](#fc) of a field contains what’s needed to decode it as
a *value*.


In a [data stream](#ds), a field is a specific [sequence](#seq-def) of
bits, whereas a *value* is the meaningful interpretation of these bits
with associated semantics.


 The types of values are:


| Value type | Possible values |
|---|---|
| Nil | None. |
| Boolean | *True* or *false*. |
| Unsigned/signed integer | Integral quantity. |
| Real | Continuous quantity. |
| String | [Sequence](#seq-def) of [Unicode](https://home.unicode.org/) codepoints. |
| Array | Sequence of values. |
| Structure | Sequence of named values (members). |


To decode an instance of a field class ***F***, depending on the
value of its `type` property:


| Value of the `type` property of ***F*** | Decoding procedure of ***F*** |
|---|---|
| [`"fixed-length-bit-array"`](#fl-ba-fc) | [Decode a fixed-length bit array field](#fl-ba-field-dec). |
| [`"fixed-length-bit-map"`](#fl-bm-fc) | [Decode a fixed-length bit map field](#fl-bm-field-dec). |
| [`"fixed-length-boolean"`](#fl-bool-fc) | [Decode a fixed-length boolean field](#fl-bool-field-dec). |
| [`"fixed-length-unsigned-integer"`](#fl-int-fc) | [Decode a fixed-length unsigned integer field](#fl-uint-field-dec). |
| [`"fixed-length-signed-integer"`](#fl-int-fc) | [Decode a fixed-length signed integer field](#fl-sint-field-dec). |
| [`"fixed-length-floating-point-number"`](#fl-fp-fc) | [Decode a fixed-length floating point number field](#fl-fp-field-dec). |
| [`"variable-length-unsigned-integer"`](#vl-int-fc) | [Decode a variable-length unsigned integer field](#vl-uint-field-dec). |
| [`"variable-length-signed-integer"`](#vl-int-fc) | [Decode a variable-length signed integer field](#vl-sint-field-dec). |
| [`"null-terminated-string"`](#nt-str-fc) | [Decode a null-terminated string field](#nt-str-field-dec). |
| [`"static-length-string"`](#sl-str-fc) | [Decode a static-length string field](#sl-str-field-dec). |
| [`"static-length-blob"`](#sl-blob-fc) | [Decode a static-length BLOB field](#sl-blob-field-dec). |
| [`"dynamic-length-string"`](#dl-str-fc) | [Decode a dynamic-length string field](#dl-str-field-dec). |
| [`"dynamic-length-blob"`](#dl-blob-fc) | [Decode a dynamic-length BLOB field](#dl-blob-field-dec). |
| [`"structure"`](#struct-fc) | [Decode a structure field](#struct-field-dec). |
| [`"static-length-array"`](#sl-array-fc) | [Decode a static-length array field](#sl-array-field-dec). |
| [`"dynamic-length-array"`](#dl-array-fc) | [Decode a dynamic-length array field](#dl-array-field-dec). |
| [`"optional"`](#opt-fc) | [Decode an optional field](#opt-field-dec). |
| [`"variant"`](#var-fc) | [Decode a variant field](#var-field-dec). |


#### 6.4.1. Alignment procedure


The decoding procedure of many fields requires ***X*** to have a
specific *alignment*.


The alignment *requirement* of an instance of a
[field class](#fc) ***F*** is, depending on the value of its
`type` property:


| `type` property of ***F*** | Alignment requirement of an instance of ***F*** |
|---|---|
| [`"fixed-length-bit-array"`](#fl-ba-fc) [`"fixed-length-bit-map"`](#fl-bm-fc) [`"fixed-length-boolean"`](#fl-bool-fc) [`"fixed-length-unsigned-integer"`](#fl-int-fc) [`"fixed-length-signed-integer"`](#fl-int-fc) [`"fixed-length-floating-point-number"`](#fl-fp-fc) | The value of the `alignment` property of ***F***. |
| [`"variable-length-unsigned-integer"`](#vl-int-fc) [`"variable-length-signed-integer"`](#vl-int-fc) [`"null-terminated-string"`](#nt-str-fc) [`"static-length-string"`](#sl-str-fc) [`"static-length-blob"`](#sl-blob-fc) [`"dynamic-length-string"`](#dl-str-fc) [`"dynamic-length-blob"`](#dl-blob-fc) | 8 |
| [`"structure"`](#struct-fc) | The *maximum* value of: The value of the `minimum-alignment` property of ***F***. The alignment requirements of the instances of the `field-class` property of each [member class](#struct-member-cls) of the `member-classes` property of ***F***. |
| [`"static-length-array"`](#sl-array-fc) [`"dynamic-length-array"`](#dl-array-fc) | The *maximum* value of: The value of the `minimum-alignment` property of ***F***. The alignment requirement of an instance of the `element-field-class` property of ***F***. |
| [`"optional"`](#opt-fc) [`"variant"`](#var-fc) | 1 |


To align ***X*** to some alignment requirement ***A***
(bits):


- Set ***X*** to
((***X*** + ***A*** − 1) & −***A***),
where “&” is the bitwise *AND* operator.

- If ***X*** > ***PKT_CONTENT_LEN***, then report an
error and abort the data stream decoding process.


#### 6.4.2. Field location procedure


To locate a previously decoded length or selector field using a
[field location](#field-loc) ***FL*** while decoding another
dependent field ***F***:


- Let ***V*** be:


If the `origin` property of ***FL*** exists

Depending on the value of the `origin` property of ***FL***:


`"packet-header"`

The [header](#pkt-header) structure of ***P***
([current packet](#pkt-dec)).

`"packet-context"`

The [context](#pkt-ctx) structure of ***P***.

`"event-record-header"`

The [header](#er-header) structure of ***E***
([current event record](#er-dec)).

`"event-record-common-context"`

The [common context](#er-common-ctx) structure of ***E***.

`"event-record-specific-context"`

The [specific context](#er-spec-ctx) structure of ***E***.

`"event-record-payload"`

The [payload](#er-payload) structure of ***E***.


If the consumer cannot set ***V*** because there’s no such
structure or because it’s not already decoded nor currently being
decoded, then report an error and abort the data stream decoding
process.


Otherwise

The most nested structure (immediate parent) containing the eventual
value of ***F***.

- For each element ***FLE*** of the `path` property
of ***FL***:


Let ***V*** be, depending on ***FLE***:


A JSON string

The value of the member named ***FLE*** within ***V***.

If no member is named ***FLE*** within ***V***, then
report an error and abort the data stream decoding process.


If the member named ***FLE*** within ***V*** isn’t already
decoded nor currently being decoded, then report an error and abort
the data stream decoding process.


`null`

The most nested structure (immediate parent)
containing ***V***.

If no structure contains ***V***, then report an error and abort the
data stream decoding process.

- Depending on the [type](#dec-val-type) of ***V***:


Boolean
Signed integer
Unsigned integer

If ***FLE*** isn’t the last element of ***FL***,
then report an error and abort the data stream decoding process.

Structure

Continue.

Array

While ***V*** is an array:


If ***V*** isn’t currently being decoded, then report an error and
abort the data stream decoding process.

Set ***V*** to the element of ***V*** currently being decoded.


Other

Report an error and abort the data stream decoding process.


***V*** is the located field.


Example 87. [Dynamic-length array](#dl-array) field and its length field in the same root field.


Assume the following JSON object is an event record payload [structure field class](#struct-fc).


```
{
  "type": "structure",
  "member-classes": [
    {
      "name": "corn", (3)
      "field-class": {
        "type": "fixed-length-unsigned-integer",
        "length": 32,
        "byte-order": "little-endian"
      }
    },
    {
      "name": "inside",
      "field-class": {
        "type": "fixed-length-unsigned-integer",
        "length": 16,
        "byte-order": "little-endian"
      }
    },
    {
      "name": "carbon",
      "field-class": {
        "type": "dynamic-length-array", (1)
        "length-field-location": { (2)
          "origin": "event-record-payload",
          "path": ["corn"]
        },
        "element-field-class": {
          "type": "null-terminated-string"
        }
      }
    }
  ]
}
```


| **1** | [Dynamic-length array field class](#dl-array-fc). |
|---|---|
| ****2** | Length field location of the [dynamic-length array field class](#dl-array-fc). |
| ****3** | Length member class. |


Example 88. Dynamic-length array field and its length field in the same root field, within the same array field element.


Assume the following JSON object is an event record payload [structure field class](#struct-fc).


Both the dynamic-length array field and its length field exist within the same
element of the [static-length array](#sl-array-fc) field named `nature`.


```
{
  "type": "structure",
  "member-classes": [
    {
      "name": "norm",
      "field-class": {
        "type": "null-terminated-string"
      }
    },
    {
      "name": "nature",
      "field-class": {
        "type": "static-length-array",
        "length": 43,
        "element-field-class": {
          "type": "structure",
          "member-classes": [
            {
              "name": "laser", (3)
              "field-class": {
                "type": "variable-length-unsigned-integer"
              }
            },
            {
              "name": "joystick",
              "field-class": {
                "type": "dynamic-length-array", (1)
                "length-field-location": { (2)
                  "path": ["laser"]
                },
                "element-field-class": {
                  "type": "null-terminated-string"
                }
              }
            }
          ]
        }
      }
    }
  ]
}
```


| ****1** | [Dynamic-length array field class](#dl-array-fc). |
|---|---|
| ****2** | Length field location of the [dynamic-length array field class](#dl-array-fc). |
| ****3** | Length member class. |


Example 89. Dynamic-length array and its length field in the same root field, within the same [variant](#var-fc) field.


Assume the following JSON object is an event record payload [structure field class](#struct-fc).


Both the dynamic-length array field and its length field exist within the same
option of the [variant](#var-fc) field named `clinic`.


Moreover, the selector field of the `clinic` variant field is the
`lawyer` field.


```
{
  "type": "structure",
  "member-classes": [
    {
      "name": "lawyer", (5)
      "field-class": {
        "type": "fixed-length-signed-integer",
        "length": 16,
        "byte-order": "little-endian"
      }
    },
    {
      "name": "clinic",
      "field-class": {
        "type": "variant",
        "selector-field-location": { (4)
          "origin": "event-record-payload",
          "path": ["lawyer"]
        },
        "options": [
          {
            "selector-field-ranges": [[0, 0]],
            "field-class": {
              "type": "null-terminated-string"
            }
          },
          {
            "selector-field-ranges": [[1, 4]],
            "field-class": {
              "type": "structure",
              "member-classes": [
                {
                  "name": "lemon", (3)
                  "field-class": {
                    "type": "fixed-length-unsigned-integer",
                    "length": 8,
                    "byte-order": "big-endian"
                  }
                },
                {
                  "name": "joystick",
                  "field-class": {
                    "type": "dynamic-length-array", (1)
                    "length-field-location": { (2)
                      "origin": "event-record-payload",
                      "path": ["clinic", "lemon"]
                    },
                    "element-field-class": {
                      "type": "null-terminated-string"
                    }
                  }
                }
              ]
            }
          },
          {
            "selector-field-ranges": [[5, 5], [7, 7]],
            "field-class": {
              "type": "fixed-length-boolean",
              "length": 8,
              "byte-order": "little-endian"
            }
          }
        ]
      }
    }
  ]
}
```


| ****1** | [Dynamic-length array field class](#dl-array-fc). |
|---|---|
| ****2** | Length field location of the [dynamic-length array field class](#dl-array-fc). |
| ****3** | Length member class. |
| ****4** | Selector field location of the variant field class. |
| ****5** | Selector member class. |


Example 90. Dynamic-length array and its length field in the same root field; length field is a variant field.


Assume the following JSON object is an event record payload [structure field class](#struct-fc).


The length field of the dynamic-length array field is a variant field: it can be
an 8-bit, a 16-bit, or a 32-bit [fixed-length integer](#fl-int-fc) field, depending
on the selection of the variant field.


Moreover, the selector field of the variant field is located in another
root field (event record specific context).


```
{
  "type": "structure",
  "member-classes": [
    {
      "name": "glass", (3)
      "field-class": {
        "type": "variant",
        "selector-field-location": {
          "origin": "event-record-specific-context",
          "path": ["sel"]
        },
        "options": [
          {
            "selector-field-ranges": [[0, 0]],
            "field-class": {
              "type": "fixed-length-unsigned-integer", (4)
              "length": 8,
              "byte-order": "little-endian"
            }
          },
          {
            "selector-field-ranges": [[1, 1]],
            "field-class": {
              "type": "fixed-length-unsigned-integer", (4)
              "length": 16,
              "byte-order": "little-endian"
            }
          },
          {
            "selector-field-ranges": [[2, 2]],
            "field-class": {
              "type": "fixed-length-unsigned-integer", (4)
              "length": 32,
              "byte-order": "little-endian"
            }
          }
        ]
      }
    },
    {
      "name": "margin",
      "field-class": {
        "type": "dynamic-length-array", (1)
        "length-field-location": { (2)
          "origin": "event-record-payload",
          "path": ["glass"]
        },
        "element-field-class": {
          "type": "null-terminated-string"
        }
      }
    }
  ]
}
```


| ****1** | [Dynamic-length array field class](#dl-array-fc). |
|---|---|
| ****2** | Length field location of the [dynamic-length array field class](#dl-array-fc). |
| ****3** | Length member class. |
| ****4** | Possible length field class. |


Example 91. Dynamic-length array and its length field in the same root field; structure field containing length field is a variant field.


Assume the following JSON object is an event record payload [structure field class](#struct-fc).


The length field of the dynamic-length array field is within a structure field
which is a variant field.


Moreover:


- The selector field of the variant field is located in another root
field (event record common context).

- The field class of the third option of the `glass` variant field class
contains a [dynamic-length BLOB field class](#dl-blob-fc) (`lock` member); the
length field of its instance is the previous member (`eagle`) within
the same structure field.


```
{
  "type": "structure",
  "member-classes": [
    {
      "name": "glass",
      "field-class": {
        "type": "variant",
        "selector-field-location": {
          "origin": "event-record-common-context",
          "path": ["sel"]
        },
        "options": [
          {
            "selector-field-ranges": [[0, 0]],
            "field-class": {
              "type": "structure",
              "member-classes": [
                {
                  "name": "eagle",
                  "field-class": {
                    "type": "fixed-length-unsigned-integer", (3)
                    "length": 16,
                    "byte-order": "little-endian"
                  }
                },
                {
                  "name": "road",
                  "field-class": {
                    "type": "null-terminated-string"
                  }
                }
              ]
            }
          },
          {
            "selector-field-ranges": [[32, 172]],
            "field-class": {
              "type": "structure",
              "member-classes": [
                {
                  "name": "nuance",
                  "field-class": {
                    "type": "null-terminated-string"
                  }
                },
                {
                  "name": "eagle",
                  "field-class": {
                    "type": "fixed-length-unsigned-integer", (3)
                    "length": 24,
                    "byte-order": "big-endian"
                  }
                }
              ]
            }
          },
          {
            "selector-field-ranges": [[5, 5]],
            "field-class": {
              "type": "structure",
              "member-classes": [
                {
                  "name": "eagle", (5)
                  "field-class": {
                    "type": "variable-length-unsigned-integer" (3)
                  }
                },
                {
                  "name": "lock",
                  "field-class": {
                    "type": "dynamic-length-blob",
                    "length-field-location": { (4)
                      "origin": "event-record-payload",
                      "path": ["glass", "eagle"]
                    }
                  }
                }
              ]
            }
          }
        ]
      }
    },
    {
      "name": "margin",
      "field-class": {
        "type": "dynamic-length-array", (1)
        "length-field-location": { (2)
          "origin": "event-record-payload",
          "path": ["glass", "eagle"]
        },
        "element-field-class": {
          "type": "null-terminated-string"
        }
      }
    }
  ]
}
```


| ****1** | [Dynamic-length array field class](#dl-array-fc). |
|---|---|
| ****2** | Length field location of the [dynamic-length array field class](#dl-array-fc). |
| ****3** | Possible length field class. |
| ****4** | Length field location of the [dynamic-length BLOB field class](#dl-blob-fc). |
| ****5** | Length field class for the [dynamic-length BLOB field class](#dl-blob-fc). |


Note that both the dynamic-length array and dynamic-length BLOB field classes have the same
length field location.


Example 92. Dynamic-length array and its length field in another root field.


Assume the following JSON objects are the event record specific context
and payload [structure field classes](#struct-fc) of the same
[event record class](#erc-frag).


The length field of the dynamic-length array field of the event record payload is
within the event record specific context.


Event record specific context field class.


```
{
  "type": "structure",
  "member-classes": [
    {
      "name": "cook",
      "field-class": {
        "type": "fixed-length-floating-point-number",
        "length": 64,
        "byte-order": "little-endian"
      }
    },
    {
      "name": "vegetable", (1)
      "field-class": {
        "type": "variable-length-unsigned-integer"
      }
    }
  ]
}
```


| ****1** | Length member class. |
|---|---|


Event record payload field class.


```
{
  "type": "structure",
  "member-classes": [
    {
      "name": "avenue",
      "field-class": {
        "type": "dynamic-length-array", (1)
        "length-field-location": { (2)
          "origin": "event-record-specific-context",
          "path": ["vegetable"]
        },
        "element-field-class": {
          "type": "null-terminated-string"
        }
      }
    },
    {
      "name": "railroad",
      "field-class": {
        "type": "null-terminated-string"
      }
    }
  ]
}
```


| ****1** | [Dynamic-length array field class](#dl-array-fc). |
|---|---|
| ****2** | Length field location of the [dynamic-length array field class](#dl-array-fc). |


#### 6.4.3. Fixed-length bit array field decoding procedure


For this whole section about decoding an instance of a
[fixed-length bit array field class](#fl-ba-fc) ***F***, let ***BYTE_ORDER*** be the value of the
`byte-order` property of ***F***.


 To read a single data stream bit from an
instance of ***F***:


- Let:


***BYTE_I*** be ***X*** / 8 (integral division;
remainder discarded).

- ***BIT_I*** be:


| ***BYTE_ORDER*** is `"big-endian"` | 7 − (***X*** mod 8) |
|---|---|
| ***BYTE_ORDER*** is `"little-endian"` | ***X*** mod 8 |


- Depending on the value of the bit at the index ***BIT_I***
within the [byte](#byte-def) at the index ***BYTE_I*** from the
beginning of ***P*** (the current packet being decoded):


| 0 | The bit value is *false*. |
|---|---|
| 1 | The bit value is *true*. |


To decode an instance of ***F***:


- Let:


***L*** be the value of the `length` property of ***F***.

- ***BIT_ORDER*** be the value of the `bit-order` property
of ***F***.

- ***V*** be an array of booleans of length ***L***.

- ***I*** be an unsigned integer initialized to 0.


- [Align X](#align-dec) according to ***F***.

- If
***X*** + ***L*** > ***PKT_CONTENT_LEN***,
then report an error and abort the data stream decoding process.

- If
***X*** mod 8 ≠ 0 and
***LAST_BYTE_ORDER*** ≠ ***BYTE_ORDER***, then
report an error and abort the data stream decoding process.

- While ***I*** < ***L***:


Let ***DI*** be an unsigned integer initialized to:


| ***BIT_ORDER*** is `"first-to-last"` | ***I*** |
|---|---|
| ***BIT_ORDER*** is `"last-to-first"` | ***L*** − ***I*** − 1 |

- Set the element at the index ***DI*** of ***V***
to the current [single bit value](#fl-ba-field-dec-bit).

- Set ***I*** to ***I*** + 1.

- Add one to ***X***.


- Set ***LAST_BYTE_ORDER*** to ***BYTE_ORDER***.


***V*** is the decoded value.


To add to the decoding procedure above, note that:


- The “reading direction” within a data stream byte depends
on ***BYTE_ORDER***.

- The “filling direction” of ***V*** depends
on ***BIT_ORDER***.


Assuming ***X*** = 0, the following diagrams show which
bit is selected by ***X*** within a 16-bit fixed-length bit array field
depending on ***BYTE_ORDER*** and ***BIT_ORDER***:


`"big-endian"` and `"last-to-first"`


`"big-endian"` and `"first-to-last"`


`"little-endian"` and `"first-to-last"`


`"little-endian"` and `"last-to-first"`


Example 93. Contiguous fixed-length bit array fields: big-endian versus little-endian.


This example shows the binary layout of contiguous big-endian and
little-endian fixed-length bit array fields.


Assume that ***X*** = 16. All the fixed-length bit array fields of this
example have an implicit one-bit [alignment requirement](#align-dec).


Considering the following [member classes](#struct-member-cls) of some
[structure field class](#struct-fc):


```
[
  {
    "name": "green",
    "field-class": {
      "type": "fixed-length-bit-array",
      "length": 3,
      "byte-order": "big-endian"
    }
  },
  {
    "name": "blue",
    "field-class": {
      "type": "fixed-length-bit-array",
      "length": 9,
      "byte-order": "big-endian"
    }
  },
  {
    "name": "yellow",
    "field-class": {
      "type": "fixed-length-bit-array",
      "length": 14,
      "byte-order": "big-endian"
    }
  },
  {
    "name": "red",
    "field-class": {
      "type": "fixed-length-bit-array",
      "length": 4,
      "byte-order": "big-endian"
    }
  }
]
```


All the fixed-length bit array classes above have an implicit last-to-first bit order.


The binary layout is as such (***X*** indicated in bytes):


Considering the following member classes of some structure field class,
the only difference with the previous sample being the values of the
`byte-order` property (and the implicit first-to-last bit order):


```
[
  {
    "name": "green",
    "field-class": {
      "type": "fixed-length-bit-array",
      "length": 3,
      "byte-order": "little-endian"
    }
  },
  {
    "name": "blue",
    "field-class": {
      "type": "fixed-length-bit-array",
      "length": 9,
      "byte-order": "little-endian"
    }
  },
  {
    "name": "yellow",
    "field-class": {
      "type": "fixed-length-bit-array",
      "length": 14,
      "byte-order": "little-endian"
    }
  },
  {
    "name": "red",
    "field-class": {
      "type": "fixed-length-bit-array",
      "length": 4,
      "byte-order": "little-endian"
    }
  }
]
```


The binary layout is as such (***X*** indicated in bytes):


Example 94. Contiguous fixed-length bit array fields: first-to-last versus last-to-first bit order.


This example shows the binary layout of contiguous big-endian fixed-length bit array
fields using the last-to-first (default) and first-to-last bit order.


Assume that ***X*** = 16. All the fixed-length bit array fields of this
example have an implicit one-bit [alignment requirement](#align-dec).


Considering the following [member classes](#struct-member-cls) of some
[structure field class](#struct-fc):


```
[
  {
    "name": "green",
    "field-class": {
      "type": "fixed-length-bit-array",
      "length": 3,
      "byte-order": "big-endian"
    }
  },
  {
    "name": "blue",
    "field-class": {
      "type": "fixed-length-bit-array",
      "length": 9,
      "byte-order": "big-endian"
    }
  },
  {
    "name": "yellow",
    "field-class": {
      "type": "fixed-length-bit-array",
      "length": 14,
      "byte-order": "big-endian"
    }
  },
  {
    "name": "red",
    "field-class": {
      "type": "fixed-length-bit-array",
      "length": 4,
      "byte-order": "big-endian"
    }
  }
]
```


All the fixed-length bit array classes above have an implicit last-to-first bit order.


The binary layout is as such (***X*** indicated in bytes):


Considering the following member classes of some structure field class,
the only difference with the previous sample being the explicit value of
the `bit-order` property:


```
[
  {
    "name": "green",
    "field-class": {
      "type": "fixed-length-bit-array",
      "length": 3,
      "byte-order": "big-endian",
      "bit-order": "first-to-last"
    }
  },
  {
    "name": "blue",
    "field-class": {
      "type": "fixed-length-bit-array",
      "length": 9,
      "byte-order": "big-endian",
      "bit-order": "first-to-last"
    }
  },
  {
    "name": "yellow",
    "field-class": {
      "type": "fixed-length-bit-array",
      "length": 14,
      "byte-order": "big-endian",
      "bit-order": "first-to-last"
    }
  },
  {
    "name": "red",
    "field-class": {
      "type": "fixed-length-bit-array",
      "length": 4,
      "byte-order": "big-endian",
      "bit-order": "first-to-last"
    }
  }
]
```


The binary layout is as such (***X*** indicated in bytes):


Example 95. Padding between fixed-length bit array fields.


This example shows how the [alignment requirement](#align-dec) of a
fixed-length bit array field can translate into padding bits to skip during the
decoding process.


Assume that ***X*** = 32.


Considering the following [member classes](#struct-member-cls) of some
[structure field class](#struct-fc):


```
[
  {
    "name": "green",
    "field-class": {
      "type": "fixed-length-bit-array",
      "length": 5,
      "byte-order": "big-endian",
      "bit-order": "first-to-last"
    }
  },
  {
    "name": "blue",
    "field-class": {
      "type": "fixed-length-bit-array",
      "length": 3,
      "byte-order": "big-endian",
      "alignment": 8
    }
  },
  {
    "name": "yellow",
    "field-class": {
      "type": "fixed-length-bit-array",
      "length": 4,
      "byte-order": "big-endian",
      "bit-order": "first-to-last",
      "alignment": 4
    }
  }
]
```


The binary layout is as such (***X*** indicated in bytes):


Considering the following member classes of some structure field class,
the only difference with the previous sample being the values of the
`byte-order` and `bit-order` properties:


```
[
  {
    "name": "green",
    "field-class": {
      "type": "fixed-length-bit-array",
      "length": 5,
      "byte-order": "little-endian",
      "bit-order": "last-to-first"
    }
  },
  {
    "name": "blue",
    "field-class": {
      "type": "fixed-length-bit-array",
      "length": 3,
      "byte-order": "little-endian",
      "alignment": 8
    }
  },
  {
    "name": "yellow",
    "field-class": {
      "type": "fixed-length-bit-array",
      "length": 4,
      "byte-order": "little-endian",
      "bit-order": "last-to-first",
      "alignment": 4
    }
  }
]
```


The binary layout is as such (***X*** indicated in bytes):


Example 96. Contiguous fixed-length bit array fields with different byte orders.


[Step 3](#fl-ba-field-dec-step-3) of the decoding procedure above
requires that a [consumer](#consumer-def) stops the data stream decoding
process if the byte order between two contiguous fixed-length bit array fields changes
when ***X*** isn’t a multiple of 8.


In other words, a given data stream [byte](#byte-def) *MUST NOT* contain
bits of two fixed-length bit array fields having different byte orders.


This example shows how contiguous fixed-length bit array fields may have different byte
orders with correct [alignment](#align-dec).


Assume that ***X*** = 16.


Considering the following [member classes](#struct-member-cls) of some
[structure field class](#struct-fc):


```
[
  {
    "name": "green",
    "field-class": {
      "type": "fixed-length-bit-array",
      "length": 3,
      "byte-order": "big-endian",
      "bit-order": "first-to-last"
    }
  },
  {
    "name": "blue",
    "field-class": {
      "type": "fixed-length-bit-array",
      "length": 5,
      "byte-order": "big-endian"
    }
  },
  {
    "name": "yellow",
    "field-class": {
      "type": "fixed-length-bit-array",
      "length": 5,
      "byte-order": "little-endian",
      "bit-order": "last-to-first"
    }
  },
  {
    "name": "orange",
    "field-class": {
      "type": "fixed-length-bit-array",
      "length": 8,
      "byte-order": "little-endian"
    }
  },
  {
    "name": "red",
    "field-class": {
      "type": "fixed-length-bit-array",
      "length": 6,
      "byte-order": "big-endian",
      "bit-order": "first-to-last",
      "alignment": 8
    }
  }
]
```


The binary layout is as such:


#### 6.4.4. Fixed-length bit map field decoding procedure


To decode an instance of a [fixed-length bit map field class](#fl-bm-fc):


- Follow [Fixed-length bit array field decoding procedure](#fl-ba-field-dec).


***V*** is the decoded value.


Example 97. Fixed-length bit map field decoding.


Consider the following [fixed-length bit map field class](#fl-bm-fc) ***F***:


```
{
  "type": "fixed-length-bit-map",
  "length": 16,
  "byte-order": "big-endian",
  "flags": {
    "RED": [
      [0, 2],
      [4, 4],
      [10, 10]
    ],
    "ORANGE": [[6, 6]],
    "GREEN": [
      [7, 7],
      [9, 9],
      [11, 11]
    ],
    "YELLOW": [
      [9, 9],
      [13, 14]
    ],
    "BLUE": [[15, 15]]
  }
}
```


Assume that ***X*** = 16 to decode the following
instance of ***F***:


Because the implicit value of the `bit-order` property of ***F***
is `"last-to-first"`, then the value of element 3 of the resulting
bit array is *false* while the value of element 4 is *true*.


The resulting bit array with the associated flags is:


The active flags are `RED`, `GREEN`, and `YELLOW` because at least one
of their bits is *true*.


#### 6.4.5. Fixed-length boolean field decoding procedure


To decode an instance of a [fixed-length boolean field class](#fl-bool-fc):


- Let ***VB*** be a boolean.

- Follow [Fixed-length bit array field decoding procedure](#fl-ba-field-dec).

- If all the elements of ***V*** are *false*, then
set ***VB*** to *false*.

Else, set ***VB*** to *true*.


***VB*** is the decoded boolean value.


#### 6.4.6. Fixed-length unsigned integer field decoding procedure


To decode an instance of a [fixed-length unsigned integer field class](#fl-int-fc):


- Let ***VI*** be an unsigned integer.

- Follow [Fixed-length bit array field decoding procedure](#fl-ba-field-dec).

- Set ***VI*** as the unsigned integer interpretation
of ***V***, where the *first* element of ***V*** is the least
significant bit.


***VI*** is the decoded unsigned integer value.


#### 6.4.7. Fixed-length signed integer field decoding procedure


To decode an instance of a [fixed-length signed integer field class](#fl-int-fc):


- Let ***VI*** be a signed integer.

- Follow [Fixed-length bit array field decoding procedure](#fl-ba-field-dec).

- Set ***VI*** as the signed integer interpretation, following the
two’s complement format, of ***V***, where the *first* element
of ***V*** is the least significant bit.


***VI*** is the decoded signed integer value.


#### 6.4.8. Fixed-length floating point number field decoding procedure


To decode an instance of a [fixed-length floating point number field class](#fl-fp-fc):


- Let ***VR*** be a real value.

- Follow [Fixed-length bit array field decoding procedure](#fl-ba-field-dec).

- Set ***VR*** to the real number interpretation, following the
[IEEE 754-2008](https://standards.ieee.org/standard/754-2008.html) binary interchange format, of ***V***, where the *first* element of ***V***
is the least significant bit (least significant bit of the mantissa)
and the last element of ***V*** is the sign bit.


***VR*** is the decoded real value.


#### 6.4.9. Variable-length unsigned integer field decoding procedure


To decode an instance of a [variable-length unsigned integer field class](#vl-int-fc) ***F***:


- Let:


***B*** be a byte.

- ***A*** be an empty array of bytes.

- ***VI*** be an unsigned integer.


- [Align X](#align-dec) according to ***F***.

- If ***X*** + 8 > ***PKT_CONTENT_LEN***,
then report an error and abort the data stream decoding process.

- Set ***B*** to the byte of ***P*** at the
offset ***X***.

- Add 8 to ***X***.

- Append ***B*** to ***A***.

- While [bit 7](#byte-def) of ***B*** is set:


If ***X*** + 8 > ***PKT_CONTENT_LEN***,
then report an error and abort the data stream decoding process.

- Set ***B*** to the byte of ***P*** at the
offset ***X***.

- Add 8 to ***X***.

- Append ***B*** to ***A***.


- Decode ***A*** as ***VI*** following the *unsigned*
[LEB128](https://en.wikipedia.org/wiki/LEB128) (ULEB128) format.


***VI*** is the decoded unsigned integer value.


Example 98. 3-byte variable-length unsigned integer field decoding.


Consider the following [variable-length unsigned integer field class](#vl-int-fc) ***F***:


```
{
  "type": "variable-length-unsigned-integer"
}
```


The following diagram shows the three bytes of an instance
of ***F*** and the resulting bits.


Note that the data bits of the last byte (in red) become the first
elements of the resulting bits because LEB128 is a little-endian
encoding.


The decoded unsigned integer value is 1,876,916.


#### 6.4.10. Variable-length signed integer field decoding procedure


To decode an instance of a [variable-length signed integer field class](#vl-int-fc) ***F***:


- Let:


***B*** be a byte.

- ***A*** be an empty array of bytes.

- ***VI*** be an unsigned integer.


- [Align X](#align-dec) according to ***F***.

- If ***X*** + 8 > ***PKT_CONTENT_LEN***,
then report an error and abort the data stream decoding process.

- Set ***B*** to the byte of ***P*** at the
offset ***X***.

- Add 8 to ***X***.

- Append ***B*** to ***A***.

- While [bit 7](#byte-def) of ***B*** is set:


If ***X*** + 8 > ***PKT_CONTENT_LEN***,
then report an error and abort the data stream decoding process.

- Set ***B*** to the byte of ***P*** at the
offset ***X***.

- Add 8 to ***X***.

- Append ***B*** to ***A***.


- Decode ***A*** as ***VI*** following the *signed*
[LEB128](https://en.wikipedia.org/wiki/LEB128) format.


***VI*** is the decoded signed integer value.


Example 99. 3-byte variable-length signed integer field decoding.


Consider the following [variable-length signed integer field class](#vl-int-fc) ***F***:


```
{
  "type": "variable-length-signed-integer"
}
```


The following diagram shows the three bytes of an instance
of ***F*** and the resulting bits.


Note that the data bits of the last byte (in red) become the first
elements of the resulting bits because LEB128 is a little-endian
encoding.


The decoded signed integer value is −220,236.


#### 6.4.11. Null-terminated string field decoding procedure


To decode an instance of a [null-terminated string field class](#nt-str-fc) ***F***:


- Let:


***UL*** be an unsigned integer initialized to, depending on the
value of the `encoding` property of ***F***:


| `utf-8` | 1 |
|---|---|
| `utf-16be` `utf-16le` | 2 |
| `utf-32be` `utf-32le` | 4 |

- ***B*** be an array of ***UL*** byte(s).

- ***A*** be an empty sequence of bytes.

- ***V*** be a string.


- [Align X](#align-dec) according to ***F***.

- If
***X*** + ***UL*** × 8 > ***PKT_CONTENT_LEN***,
then report an error and abort the data stream decoding process.

- Set ***B*** to the next ***UL*** byte(s) of data
from ***P*** at the offset ***X***.

- Add ***UL*** × 8 to ***X***.

- While at least one bit within the byte(s) of ***B*** is set:


Append the byte(s) of ***B*** to ***A***.

- If
***X*** + ***UL*** × 8 > ***PKT_CONTENT_LEN***,
then report an error and abort the data stream decoding process.

- Set ***B*** to the next ***UL*** byte(s) of data
from ***P*** at the offset ***X***.

- Add ***UL*** × 8 to ***X***.


- Decode ***A*** as ***V*** following, depending on the value of the
`encoding` property of ***F***:


| `utf-8` | [UTF-8](https://en.wikipedia.org/wiki/UTF-8). |
|---|---|
| `utf-16be` | [UTF-16BE](https://en.wikipedia.org/wiki/UTF-16) (big endian). |
| `utf-16le` | [UTF-16LE](https://en.wikipedia.org/wiki/UTF-16) (little endian). |
| `utf-32be` | [UTF-32BE](https://en.wikipedia.org/wiki/UTF-32) (big endian). |
| `utf-32le` | [UTF-32LE](https://en.wikipedia.org/wiki/UTF-32) (little endian). |


***V*** is the decoded string value.


Example 100. 22-byte UTF-8 null-terminated string field decoding.


Consider the following [null-terminated string field class](#nt-str-fc) ***F***:


```
{
  "type": "null-terminated-string"
}
```


Instances of ***F*** have an implicit UTF-8 encoding.


The following diagram shows [packet](#pkt) bytes including a 22-byte
instance of ***F*** (in blue) and its resulting string value.


The offset of the null-terminated string field, from the beginning of the packet, is
0xfc23 bytes, which means ***X*** = 516,376.


The field contains 22 UTF-8 bytes, including the trailing
“NULL” (U+0000) codepoint.


The resulting string value contains 19 Unicode codepoints.


After the field is decoded, ***X*** = 516,552.


Example 101. 20-byte UTF-16LE null-terminated string field decoding.


Consider the following [null-terminated string field class](#nt-str-fc) ***F***:


```
{
  "type": "null-terminated-string",
  "encoding": "utf-16le"
}
```


Instances of ***F*** have a UTF-16LE encoding.


The following diagram shows [packet](#pkt) bytes including a 20-byte
instance of ***F*** (in blue) and its resulting string value.


The offset of the null-terminated string field, from the beginning of the packet, is
0xfc23 bytes, which means ***X*** = 516,376.


The field contains 20 UTF-16LE bytes, including the trailing
“NULL” (U+0000) codepoint.


The resulting string value contains nine Unicode codepoints.


After the field is decoded, ***X*** = 516,536.


#### 6.4.12. Static-length string field decoding procedure


To decode an instance of a [static-length string field class](#sl-str-fc) ***F***:


- Let:


***UL*** be an unsigned integer initialized to, depending on the value
of the `encoding` property of ***F***:


| `utf-8` | 1 |
|---|---|
| `utf-16be` `utf-16le` | 2 |
| `utf-32be` `utf-32le` | 4 |

- ***L*** be the `length` property of ***F***.

- ***B*** be an array of ***UL*** byte(s).

- ***A*** be an empty sequence of bytes.

- ***V*** be a string.


- [Align X](#align-dec) according to ***F***.

- If
***X*** + ***L*** × 8 > ***PKT_CONTENT_LEN***,
then report an error and abort the data stream decoding process.

- Let ***XI*** be ***X***.

- While ***X*** < ***XI*** + ***L*** × 8:


If ***X*** + ***UL*** × 8 > ***XI*** + ***L*** × 8,
then report an error and abort the data stream decoding process.

- Set ***B*** to the next ***UL*** byte(s) of data
from ***P*** at the offset ***X***.

- Add ***UL*** × 8 to ***X***.

- If at least one bit of ***B*** is set, then

Append the byte(s) of ***B*** to ***A***.

Else

Set ***X*** to ***XI*** + ***L*** × 8.


- Decode ***A*** as ***V*** following, depending on the value of the
`encoding` property of ***F***:


| `utf-8` | [UTF-8](https://en.wikipedia.org/wiki/UTF-8). |
|---|---|
| `utf-16be` | [UTF-16BE](https://en.wikipedia.org/wiki/UTF-16) (big endian). |
| `utf-16le` | [UTF-16LE](https://en.wikipedia.org/wiki/UTF-16) (little endian). |
| `utf-32be` | [UTF-32BE](https://en.wikipedia.org/wiki/UTF-32) (big endian). |
| `utf-32le` | [UTF-32LE](https://en.wikipedia.org/wiki/UTF-32) (little endian). |


***V*** is the decoded string value.


Example 102. 18-byte UTF-8 static-length string field decoding.


Consider the following [static-length string field class](#sl-str-fc) ***F***:


```
{
  "type": "static-length-string",
  "length": 18
}
```


The following diagram shows [packet](#pkt) bytes including an 18-byte
instance of ***F*** (in green) and its resulting string value.


The offset of the static-length string field, from the beginning of the packet, is
0x8c46 bytes, which means ***X*** = 287,280.


The field contains 14 UTF-8 bytes, a null terminating byte,
and three garbage data bytes to ignore.


The resulting string value contains seven Unicode codepoints.


After the field is decoded, ***X*** = 287,424.


#### 6.4.13. Static-length BLOB field decoding procedure


To decode an instance of a [static-length BLOB field class](#sl-blob-fc) ***F***:


- Let:


***L*** be the `length` property of ***F***.

- ***V*** be an array of bytes of length ***L***.


- [Align X](#align-dec) according to ***F***.

- If
***X*** + ***L*** × 8 > ***PKT_CONTENT_LEN***,
then report an error and abort the data stream decoding process.

- Read ***L*** bytes of data from ***P*** at the
offset ***X*** as ***V***.

- Add ***L*** × 8 to ***X***.


***V*** is the decoded BLOB value.


#### 6.4.14. Dynamic-length string field decoding procedure


To decode an instance of a [dynamic-length string field class](#dl-str-fc) ***F***:


- Let:


***UL*** be an unsigned integer initialized to, depending on the
value of the `encoding` property of ***F***:


| `utf-8` | 1 |
|---|---|
| `utf-16be` `utf-16le` | 2 |
| `utf-32be` `utf-32le` | 4 |

- ***L*** be the value of the previously decoded unsigned integer
field of which the value of the `length-field-location` property
of ***F*** indicates the [location](#field-loc-dec).

- ***B*** be an array of ***UL*** byte(s).

- ***A*** be an empty sequence of bytes.

- ***V*** be a string.


- [Align X](#align-dec) according to ***F***.

- If
***X*** + ***L*** × 8 > ***PKT_CONTENT_LEN***,
then report an error and abort the data stream decoding process.

- Let ***XI*** be ***X***.

- While ***X*** < ***XI*** + ***L*** × 8:


If ***X*** + ***UL*** × 8 > ***XI*** + ***L*** × 8,
then report an error and abort the data stream decoding process.

- Set ***B*** to the next ***UL*** byte(s) of data
from ***P*** at the offset ***X***.

- Add ***UL*** × 8 to ***X***.

- If at least one bit of ***B*** is set, then

Append the byte(s) of ***B*** to ***A***.

Else

Set ***X*** to ***XI*** + ***L*** × 8.


- Decode ***A*** as ***V*** following, depending on the value of the
`encoding` property of ***F***:


| `utf-8` | [UTF-8](https://en.wikipedia.org/wiki/UTF-8). |
|---|---|
| `utf-16be` | [UTF-16BE](https://en.wikipedia.org/wiki/UTF-16) (big endian). |
| `utf-16le` | [UTF-16LE](https://en.wikipedia.org/wiki/UTF-16) (little endian). |
| `utf-32be` | [UTF-32BE](https://en.wikipedia.org/wiki/UTF-32) (big endian). |
| `utf-32le` | [UTF-32LE](https://en.wikipedia.org/wiki/UTF-32) (little endian). |


***V*** is the decoded string value.


#### 6.4.15. Dynamic-length BLOB field decoding procedure


To decode an instance of a [dynamic-length BLOB field class](#dl-blob-fc) ***F***:


- Let:


***L*** be the value of the previously decoded unsigned integer
field of which the value of the `length-field-location` property
of ***F*** indicates the [location](#field-loc-dec).

- ***V*** be an array of bytes of length ***L***.


- [Align X](#align-dec) according to ***F***.

- If
***X*** + ***L*** × 8 > ***PKT_CONTENT_LEN***,
then report an error and abort the data stream decoding process.

- Read ***L*** bytes of data from ***P*** at the
offset ***X*** as ***V***.

- Add ***L*** × 8 to ***X***.


***V*** is the decoded BLOB value.


#### 6.4.16. Structure field decoding procedure


To decode an instance of a [structure field class](#struct-fc) ***F***:


- Let:


***M*** be the `member-classes` property of ***F***.

- ***V*** be an empty structure.


- [Align X](#align-dec) according to ***F***.

- For each member class ***MC*** of ***M***:


Let:


***MF*** be the `field-class` property of ***MC***.

- ***MN*** be the `name` property of ***MC***.


- [Decode](#field-dec) one instance of ***MF***, appending the
resulting value as a member of ***V*** named ***MN***.


***V*** is the decoded value.


#### 6.4.17. Static-length array field decoding procedure


To decode an instance of a [static-length array field class](#sl-array-fc) ***F***:


- Let:


***L*** be the `length` property of ***F***.

- ***EF*** be the `element-field-class` property of ***F***.

- ***I*** be an unsigned integer initialized to 0.

- ***V*** be an array of values of length ***L***.


- [Align X](#align-dec) according to ***F***.

- While ***I*** < ***L***:


[Decode](#field-dec) one instance of ***EF***
as element ***I*** of ***V***.

- Add one to ***I***.


***V*** is the decoded value.


#### 6.4.18. Dynamic-length array field decoding procedure


To decode an instance of a [dynamic-length array field class](#dl-array-fc) ***F***:


- Let:


***L*** be the value of the previously decoded unsigned integer field
of which the value of the `length-field-location` property
of ***F*** indicates the [location](#field-loc-dec).

- ***EF*** be the `element-field-class` property of ***F***.

- ***I*** be an unsigned integer initialized to 0.

- ***V*** be an array of values of length ***L***


- [Align X](#align-dec) according to ***F***.

- While ***I*** < ***L***:


[Decode](#field-dec) one instance of ***EF***
as element ***I*** of ***V***.

- Add one to ***I***.


***V*** is the decoded value.


Example 103. 5-element dynamic-length array field decoding.


Consider the following [event record payload](#er-payload)
[structure field class](#struct-fc) ***F***:


```
{
  "type": "structure",
  "member-classes": [
    {
      "name": "len",
      "field-class": {
        "type": "fixed-length-unsigned-integer",
        "length": 16,
        "byte-order": "big-endian"
      }
    },
    {
      "name": "id",
      "field-class": {
        "type": "null-terminated-string"
      }
    },
    {
      "name": "vals",
      "field-class": {
        "type": "dynamic-length-array",
        "length-field-location": {
          "origin": "event-record-payload",
          "path": ["len"]
        },
        "element-field-class": {
          "type": "fixed-length-unsigned-integer",
          "length": 32,
          "byte-order": "little-endian",
          "alignment": 32
        }
      }
    }
  ]
}
```


The following diagram shows [packet](#pkt) bytes including an instance
of ***F*** (starting in green and ending in blue) and the resulting
unsigned integer values of its `vals` member.


The offset of the `len` fixed-length unsigned integer field, from the beginning of the packet, is
0x42c1 bytes, which means ***X*** = 136,712.


The offset of the `vals` dynamic-length array field, from the beginning of the
packet, is 0x42c8 bytes, which means
***X*** = 136,768.


The unsigned integer value of the `len` field is 5, which means the
`vals` field contains five fixed-length unsigned integer fields.


After the structure field is decoded, ***X*** = 136,928.


#### 6.4.19. Optional field decoding procedure


To decode an instance of an [optional field class](#opt-fc) ***F***:


- Let:


***SEL*** be the value of the previously decoded [boolean](#bool-fc)
or [integer](#int-fc) field of which the value of the
`selector-field-location` property of ***F*** indicates the
[location](#field-loc-dec).

- ***OF*** be the `field-class` property of ***F***.

- ***V*** be a nil value.


- If ***SEL*** is a boolean value
and ***SEL*** is *true*, then:


[Decode](#field-dec) one instance of ***OF***
as ***V***.


Else, if _**SEL**_ is an integer value
and _**SEL**_ is an element of any [integer range](#int-range-set)
of the `selector-field-ranges` property of ***F***, then:


- [Decode](#field-dec) one instance of ***OF***
as ***V***.


***V*** is the decoded value.


#### 6.4.20. Variant field decoding procedure


To decode an instance of an [variant field class](#var-fc) ***F***:


- Let:


***SEL*** be the value of the previously decoded [integer](#int-fc)
field of which the value of the `selector-field-location` property
of ***F*** indicates the [location](#field-loc-dec).

- ***OPTS*** be the `options` property of ***F***.

- ***V*** be a value.


- Let ***OPT*** be the [variant field class option](#var-fc-opt)
of ***OPTS*** of which ***SEL*** is an element of any
[integer range](#int-range-set) of its `selector-field-ranges`
property.

If no option of ***OPTS*** has such an integer range, then report
an error and abort the data stream decoding process.

- Let ***OF*** be the `field-class` property of ***OPT***.

- [Decode](#field-dec) one instance of ***OF***
as ***V***.


***V*** is the decoded value.

