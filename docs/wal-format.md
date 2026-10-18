# WAL Binary Record Format

## 1. Overview

The WAL stores database operations as serialized binary records. Each record contains operation metadata, a key-value payload, and a CRC-32 checksum used to detect accidental corruption.

The format uses fixed-width integer fields and variable-length key and value fields.

## 2. Record Layout

All integer fields are encoded in **big-endian byte order**.

| Field | Size (bytes) | Description |
|---|---:|---|
| Payload length | 4 | Number of bytes following this field, including the checksum |
| Record type | 1 | Operation or transaction record type |
| Sequence number | 8 | Sequence number associated with the record |
| Transaction ID | 8 | Identifier of the associated transaction |
| Timestamp | 8 | Timestamp associated with the record |
| Key length | 4 | Length of the key in bytes |
| Key | Variable | Key bytes |
| Value length | 4 | Length of the value in bytes |
| Value | Variable | Value bytes |
| CRC-32 checksum | 4 | Checksum of the prefix and record contents preceding this field |

The fixed payload overhead is 37 bytes, excluding the 4-byte payload-length prefix. Therefore:

`Payload length = 37 + key length + value length`

`Total record size = 4 + payload length`

The maximum payload length is 16 MiB (16 × 1024 × 1024 bytes). The 4-byte prefix is not included in this limit.

## 3. Record Types

The record type is encoded as one byte using the numeric value of `storage::RecordType`.

| Type | Meaning |
|---|---|
| `PUT` | Insert or update a key-value pair |
| `DELETE` | Delete a key |
| `BEGIN` | Begin a transaction |
| `COMMIT` | Commit a transaction |

The current enum values are assigned in declaration order, starting at zero.

## 4. Key and Value Encoding

Keys and values are stored as raw byte sequences. Their lengths are specified explicitly in bytes.

Empty keys and values are representable at the serialization layer. Embedded null bytes are also supported; the data must not be interpreted as null-terminated strings when reading or writing the record.

Application-level rules may impose additional restrictions on which record types can contain keys or values.

## 5. Checksum

The checksum uses the CRC-32/ISO-HDLC algorithm.

- Initial value: `0xFFFFFFFF`
- Reflected polynomial: `0xEDB88320`
- Final XOR value: `0xFFFFFFFF`

The checksum covers the 4-byte payload-length prefix and every subsequent record byte before the checksum field. The checksum field itself is excluded.

The checksum detects accidental corruption; it is not a cryptographic integrity mechanism.

## 6. Deserialization Validation

A record is rejected if any of the following conditions holds:

- The input is too short to contain the payload-length prefix.
- The payload length is below the fixed overhead or exceeds the maximum.
- The declared payload length does not match the actual input size.
- The record type is unknown.
- The key or value lengths are inconsistent with the available bytes.
- The stored checksum does not match the calculated checksum.

Deserialization returns no record for invalid input.

## 7. Timestamp Convention

The timestamp field is an unsigned 64-bit integer representing the number of microseconds elapsed since the Unix epoch: 1970-01-01 00:00:00 UTC.

Timestamps are expressed in UTC and use Unix epoch microseconds, regardless of the server's local time zone.

The timestamp records the time associated with the operation. The component responsible for creating WAL records must use this convention consistently.

Timestamps are not guaranteed to be unique. Multiple operations may share the same timestamp, and the timestamp alone must not be used to establish a strict ordering between records. The sequence number provides the record-ordering mechanism.

Rollback to a requested timestamp must use a consistently defined comparison rule. If multiple records have the same timestamp, the recovery implementation must use the sequence number as a tie-breaker where needed.

## 8. Format Compatibility

Changes to field order, field widths, byte order, record-type numeric values, length interpretation, or checksum rules can make existing WAL files incompatible.

Such changes must be coordinated across serialization, deserialization, and recovery code.