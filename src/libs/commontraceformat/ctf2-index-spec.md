# LTTng CTF Index File Format

The `.idx` index files are an **LTTng convention**, not part of the CTF 2 specification. They enable efficient time-range seeking in large stream files without scanning the binary data.

## File Layout

```
[ Header (16 bytes) ]
[ Entry 0 (entry_size bytes) ]
[ Entry 1 (entry_size bytes) ]
...
```

## Header (16 bytes, big-endian)

| Offset | Size | Field | Value |
|---|---|---|---|
| 0 | 4 | magic | `0xC1F1DCC1` |
| 4 | 4 | major | `1` |
| 8 | 4 | minor | `1` |
| 12 | 4 | entry_size | size of each entry in bytes (currently `72`) |

## Entry (72 bytes, big-endian)

One entry per packet in the corresponding stream file:

| Offset | Size | Field | Description |
|---|---|---|---|
| 0 | 8 | `offset_bytes` | Byte offset of the packet in the stream file — enables direct `seek()` |
| 8 | 8 | `packet_size_bits` | Total size of the packet in bits |
| 16 | 8 | `content_size_bits` | Content size of the packet in bits (excludes trailing padding) |
| 24 | 8 | `ts_begin` | Timestamp of the first event in the packet |
| 32 | 8 | `ts_end` | Timestamp of the last event in the packet |
| 40 | 8 | `events_discarded` | Cumulative count of discarded events up to this packet |
| 48 | 8 | `stream_class_id` | Numeric ID of the data stream class |
| 56 | 8 | `stream_id` | Numeric ID of the data stream instance |
| 64 | 8 | `packet_seq_num` | Packet sequence number within the stream |

## Purpose

**Time-range seeking without scanning**: to view events between two timestamps in a large trace, binary-search the index by `ts_begin`/`ts_end` to find the relevant packet offsets, then `seek()` directly to them in the stream file. Without the index, every packet from the beginning of the file would need to be decoded.

Additional uses:
- **Packet boundary detection**: `packet_size_bits` and `content_size_bits` are available without decoding the packet header, which is useful when packet header field classes vary.
- **Gap detection**: `packet_seq_num` lets a consumer identify missing packets without decoding.
- **Discarded event accounting**: `events_discarded` provides the running total at each packet boundary.
- **Stream identification**: `stream_class_id` and `stream_id` identify which logical stream a packet belongs to, relevant when multiple stream files share a DSC.

## File Naming Convention

Index files live in an `index/` subdirectory alongside the stream files, named after the stream file with a `.idx` suffix:

```
trace/
  metadata
  ust_channel_0
  ust_channel_1
  index/
    ust_channel_0.idx
    ust_channel_1.idx
```

For tracefile-rotation traces (where one logical stream is split across multiple files), each rotation file has its own index:

```
kernel/
  metadata
  mychan_0_0
  mychan_0_1
  mychan_0_2
  index/
    mychan_0_0.idx
    mychan_0_1.idx
    mychan_0_2.idx
```
