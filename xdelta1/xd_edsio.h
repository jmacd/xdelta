/* -*-Mode: C;-*-
 * Copyright (C) 1997, 1998, 1999  Josh MacDonald
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * Author: Josh MacDonald <jmacd@CS.Berkeley.EDU>
 *
 * This file was AUTOMATICALLY GENERATED using:
 *
 * $Id: edsio.el 1.16 Tue, 06 Apr 1999 23:40:10 -0700 jmacd $
 */

#include "edsio.h"

#include "xdelta.h"

#ifndef _XD_EDSIO_H_
#define _XD_EDSIO_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "edsio_edsio.h"

/* Initialize this library. */

gboolean xd_edsio_init (void);

/* Types defined here. */

/* XdStringEventCode.
 */

typedef struct _XdStringEventCode XdStringEventCode;
struct _XdStringEventCode { gint code; };

typedef struct _XdStringEvent XdStringEvent;
struct _XdStringEvent { gint code; const char* srcfile; guint srcline; const char* version; };

/* XdHandleIntIntEventCode.
 */

typedef struct _XdHandleIntIntEventCode XdHandleIntIntEventCode;
struct _XdHandleIntIntEventCode { gint code; };

typedef struct _XdHandleIntIntEvent XdHandleIntIntEvent;
struct _XdHandleIntIntEvent { gint code; const char* srcfile; guint srcline; FileHandle* stream; int expected; int received; };

/* XdHandleStringStringEventCode.
 */

typedef struct _XdHandleStringStringEventCode XdHandleStringStringEventCode;
struct _XdHandleStringStringEventCode { gint code; };

typedef struct _XdHandleStringStringEvent XdHandleStringStringEvent;
struct _XdHandleStringStringEvent { gint code; const char* srcfile; guint srcline; FileHandle* stream; const char* expected; const char* received; };

/* XdIntEventCode.
 */

typedef struct _XdIntEventCode XdIntEventCode;
struct _XdIntEventCode { gint code; };

typedef struct _XdIntEvent XdIntEvent;
struct _XdIntEvent { gint code; const char* srcfile; guint srcline; int index; };

/* XdVoidEventCode.
 */

typedef struct _XdVoidEventCode XdVoidEventCode;
struct _XdVoidEventCode { gint code; };

typedef struct _XdVoidEvent XdVoidEvent;
struct _XdVoidEvent { gint code; const char* srcfile; guint srcline; };

typedef struct _SerialVersion0Instruction SerialVersion0Instruction;
typedef struct _SerialVersion0Control SerialVersion0Control;
typedef struct _SerialVersion0SourceInfo SerialVersion0SourceInfo;
typedef struct _SerialRsyncIndex SerialRsyncIndex;
typedef struct _SerialRsyncIndexElt SerialRsyncIndexElt;
typedef struct _SerialXdeltaInstruction SerialXdeltaInstruction;
typedef struct _SerialXdeltaControl SerialXdeltaControl;
typedef struct _SerialXdeltaSourceInfo SerialXdeltaSourceInfo;
typedef struct _SerialXdeltaIndex SerialXdeltaIndex;
typedef struct _SerialXdeltaChecksum SerialXdeltaChecksum;
/* Functions declared here. */

/* Serial Types */

enum _SerialXdType {

  ST_XdeltaChecksum = (1<<(1+EDSIO_LIBRARY_OFFSET_BITS))+3, 
  ST_XdeltaIndex = (1<<(2+EDSIO_LIBRARY_OFFSET_BITS))+3, 
  ST_XdeltaSourceInfo = (1<<(3+EDSIO_LIBRARY_OFFSET_BITS))+3, 
  ST_XdeltaControl = (1<<(7+EDSIO_LIBRARY_OFFSET_BITS))+3, 
  ST_XdeltaInstruction = (1<<(8+EDSIO_LIBRARY_OFFSET_BITS))+3, 
  ST_RsyncIndexElt = (1<<(9+EDSIO_LIBRARY_OFFSET_BITS))+3, 
  ST_RsyncIndex = (1<<(10+EDSIO_LIBRARY_OFFSET_BITS))+3, 
  ST_Version0SourceInfo = (1<<(4+EDSIO_LIBRARY_OFFSET_BITS))+3, 
  ST_Version0Control = (1<<(5+EDSIO_LIBRARY_OFFSET_BITS))+3, 
  ST_Version0Instruction = (1<<(6+EDSIO_LIBRARY_OFFSET_BITS))+3
};



/* XdeltaChecksum Structure
 */

struct _SerialXdeltaChecksum {
  guint16 high;
  guint16 low;
};

void     serializeio_print_xdeltachecksum_obj        (SerialXdeltaChecksum* obj, guint indent_spaces);

gboolean unserialize_xdeltachecksum                  (SerialSource *source, SerialXdeltaChecksum**);
gboolean unserialize_xdeltachecksum_internal         (SerialSource *source, SerialXdeltaChecksum** );
gboolean unserialize_xdeltachecksum_internal_noalloc (SerialSource *source, SerialXdeltaChecksum* );
gboolean serialize_xdeltachecksum                    (SerialSink *sink, guint16 high, guint16 low);
gboolean serialize_xdeltachecksum_obj                (SerialSink *sink, const SerialXdeltaChecksum* obj);
gboolean serialize_xdeltachecksum_internal           (SerialSink *sink, guint16 high, guint16 low);
gboolean serialize_xdeltachecksum_obj_internal (SerialSink *sink, SerialXdeltaChecksum* obj);
guint    serializeio_count_xdeltachecksum            (guint16 high, guint16 low);
guint    serializeio_count_xdeltachecksum_obj        (SerialXdeltaChecksum const* obj);

/* XdeltaIndex Structure
 */

struct _SerialXdeltaIndex {
  guint32 file_len;
  guint8 file_md5[16];
  guint32 index_len;
  SerialXdeltaChecksum* index;
};

void     serializeio_print_xdeltaindex_obj        (SerialXdeltaIndex* obj, guint indent_spaces);

gboolean unserialize_xdeltaindex                  (SerialSource *source, SerialXdeltaIndex**);
gboolean unserialize_xdeltaindex_internal         (SerialSource *source, SerialXdeltaIndex** );
gboolean unserialize_xdeltaindex_internal_noalloc (SerialSource *source, SerialXdeltaIndex* );
gboolean serialize_xdeltaindex                    (SerialSink *sink, guint32 file_len, const guint8 file_md5[16], guint32 index_len, SerialXdeltaChecksum const* index);
gboolean serialize_xdeltaindex_obj                (SerialSink *sink, const SerialXdeltaIndex* obj);
gboolean serialize_xdeltaindex_internal           (SerialSink *sink, guint32 file_len, const guint8 file_md5[16], guint32 index_len, SerialXdeltaChecksum const* index);
gboolean serialize_xdeltaindex_obj_internal (SerialSink *sink, SerialXdeltaIndex* obj);
guint    serializeio_count_xdeltaindex            (guint32 file_len, const guint8 file_md5[16], guint32 index_len, SerialXdeltaChecksum const* index);
guint    serializeio_count_xdeltaindex_obj        (SerialXdeltaIndex const* obj);

/* XdeltaSourceInfo Structure
 */

struct _SerialXdeltaSourceInfo {
  const gchar* name;
  guint8 md5[16];
  guint32 len;
  gboolean isdata;
  gboolean sequential;
  guint32       position;
  guint32       copies;
  guint32       copy_length;
  FileHandle   *in;
};

void     serializeio_print_xdeltasourceinfo_obj        (SerialXdeltaSourceInfo* obj, guint indent_spaces);

gboolean unserialize_xdeltasourceinfo                  (SerialSource *source, SerialXdeltaSourceInfo**);
gboolean unserialize_xdeltasourceinfo_internal         (SerialSource *source, SerialXdeltaSourceInfo** );
gboolean unserialize_xdeltasourceinfo_internal_noalloc (SerialSource *source, SerialXdeltaSourceInfo* );
gboolean serialize_xdeltasourceinfo                    (SerialSink *sink, const gchar* name, const guint8 md5[16], guint32 len, gboolean isdata, gboolean sequential);
gboolean serialize_xdeltasourceinfo_obj                (SerialSink *sink, const SerialXdeltaSourceInfo* obj);
gboolean serialize_xdeltasourceinfo_internal           (SerialSink *sink, const gchar* name, const guint8 md5[16], guint32 len, gboolean isdata, gboolean sequential);
gboolean serialize_xdeltasourceinfo_obj_internal (SerialSink *sink, SerialXdeltaSourceInfo* obj);
guint    serializeio_count_xdeltasourceinfo            (const gchar* name, const guint8 md5[16], guint32 len, gboolean isdata, gboolean sequential);
guint    serializeio_count_xdeltasourceinfo_obj        (SerialXdeltaSourceInfo const* obj);

/* XdeltaControl Structure
 */

struct _SerialXdeltaControl {
  guint8 to_md5[16];
  guint32 to_len;
  gboolean has_data;
  guint32 source_info_len;
  SerialXdeltaSourceInfo** source_info;
  guint32 inst_len;
  SerialXdeltaInstruction* inst;
  GArray    *inst_array;
  GPtrArray *source_info_array;
};

void     serializeio_print_xdeltacontrol_obj        (SerialXdeltaControl* obj, guint indent_spaces);

gboolean unserialize_xdeltacontrol                  (SerialSource *source, SerialXdeltaControl**);
gboolean unserialize_xdeltacontrol_internal         (SerialSource *source, SerialXdeltaControl** );
gboolean unserialize_xdeltacontrol_internal_noalloc (SerialSource *source, SerialXdeltaControl* );
gboolean serialize_xdeltacontrol                    (SerialSink *sink, const guint8 to_md5[16], guint32 to_len, gboolean has_data, guint32 source_info_len, SerialXdeltaSourceInfo* const* source_info, guint32 inst_len, SerialXdeltaInstruction const* inst);
gboolean serialize_xdeltacontrol_obj                (SerialSink *sink, const SerialXdeltaControl* obj);
gboolean serialize_xdeltacontrol_internal           (SerialSink *sink, const guint8 to_md5[16], guint32 to_len, gboolean has_data, guint32 source_info_len, SerialXdeltaSourceInfo* const* source_info, guint32 inst_len, SerialXdeltaInstruction const* inst);
gboolean serialize_xdeltacontrol_obj_internal (SerialSink *sink, SerialXdeltaControl* obj);
guint    serializeio_count_xdeltacontrol            (const guint8 to_md5[16], guint32 to_len, gboolean has_data, guint32 source_info_len, SerialXdeltaSourceInfo* const* source_info, guint32 inst_len, SerialXdeltaInstruction const* inst);
guint    serializeio_count_xdeltacontrol_obj        (SerialXdeltaControl const* obj);

/* XdeltaInstruction Structure
 */

struct _SerialXdeltaInstruction {
  guint32 index;
  guint32 offset;
  guint32 length;
  guint32 output_start;
};

void     serializeio_print_xdeltainstruction_obj        (SerialXdeltaInstruction* obj, guint indent_spaces);

gboolean unserialize_xdeltainstruction                  (SerialSource *source, SerialXdeltaInstruction**);
gboolean unserialize_xdeltainstruction_internal         (SerialSource *source, SerialXdeltaInstruction** );
gboolean unserialize_xdeltainstruction_internal_noalloc (SerialSource *source, SerialXdeltaInstruction* );
gboolean serialize_xdeltainstruction                    (SerialSink *sink, guint32 index, guint32 offset, guint32 length);
gboolean serialize_xdeltainstruction_obj                (SerialSink *sink, const SerialXdeltaInstruction* obj);
gboolean serialize_xdeltainstruction_internal           (SerialSink *sink, guint32 index, guint32 offset, guint32 length);
gboolean serialize_xdeltainstruction_obj_internal (SerialSink *sink, SerialXdeltaInstruction* obj);
guint    serializeio_count_xdeltainstruction            (guint32 index, guint32 offset, guint32 length);
guint    serializeio_count_xdeltainstruction_obj        (SerialXdeltaInstruction const* obj);

/* RsyncIndexElt Structure
 */

struct _SerialRsyncIndexElt {
  guint8 md5[16];
  SerialXdeltaChecksum cksum;
  SerialRsyncIndexElt* next;
  gint match_offset;
};

void     serializeio_print_rsyncindexelt_obj        (SerialRsyncIndexElt* obj, guint indent_spaces);

gboolean unserialize_rsyncindexelt                  (SerialSource *source, SerialRsyncIndexElt**);
gboolean unserialize_rsyncindexelt_internal         (SerialSource *source, SerialRsyncIndexElt** );
gboolean unserialize_rsyncindexelt_internal_noalloc (SerialSource *source, SerialRsyncIndexElt* );
gboolean serialize_rsyncindexelt                    (SerialSink *sink, const guint8 md5[16], SerialXdeltaChecksum const* cksum);
gboolean serialize_rsyncindexelt_obj                (SerialSink *sink, const SerialRsyncIndexElt* obj);
gboolean serialize_rsyncindexelt_internal           (SerialSink *sink, const guint8 md5[16], SerialXdeltaChecksum const* cksum);
gboolean serialize_rsyncindexelt_obj_internal (SerialSink *sink, SerialRsyncIndexElt* obj);
guint    serializeio_count_rsyncindexelt            (const guint8 md5[16], SerialXdeltaChecksum const* cksum);
guint    serializeio_count_rsyncindexelt_obj        (SerialRsyncIndexElt const* obj);

/* RsyncIndex Structure
 */

struct _SerialRsyncIndex {
  guint32 seg_len;
  guint32 file_len;
  guint8 file_md5[16];
  guint32 index_len;
  SerialRsyncIndexElt* index;
  SerialRsyncIndexElt** table;
  guint table_size;
};

void     serializeio_print_rsyncindex_obj        (SerialRsyncIndex* obj, guint indent_spaces);

gboolean unserialize_rsyncindex                  (SerialSource *source, SerialRsyncIndex**);
gboolean unserialize_rsyncindex_internal         (SerialSource *source, SerialRsyncIndex** );
gboolean unserialize_rsyncindex_internal_noalloc (SerialSource *source, SerialRsyncIndex* );
gboolean serialize_rsyncindex                    (SerialSink *sink, guint32 seg_len, guint32 file_len, const guint8 file_md5[16], guint32 index_len, SerialRsyncIndexElt const* index);
gboolean serialize_rsyncindex_obj                (SerialSink *sink, const SerialRsyncIndex* obj);
gboolean serialize_rsyncindex_internal           (SerialSink *sink, guint32 seg_len, guint32 file_len, const guint8 file_md5[16], guint32 index_len, SerialRsyncIndexElt const* index);
gboolean serialize_rsyncindex_obj_internal (SerialSink *sink, SerialRsyncIndex* obj);
guint    serializeio_count_rsyncindex            (guint32 seg_len, guint32 file_len, const guint8 file_md5[16], guint32 index_len, SerialRsyncIndexElt const* index);
guint    serializeio_count_rsyncindex_obj        (SerialRsyncIndex const* obj);

/* Version0SourceInfo Structure
 */

struct _SerialVersion0SourceInfo {
  guint8 md5[16];
  guint8 real_md5[16];
  guint32 length;
};

void     serializeio_print_version0sourceinfo_obj        (SerialVersion0SourceInfo* obj, guint indent_spaces);

gboolean unserialize_version0sourceinfo                  (SerialSource *source, SerialVersion0SourceInfo**);
gboolean unserialize_version0sourceinfo_internal         (SerialSource *source, SerialVersion0SourceInfo** );
gboolean unserialize_version0sourceinfo_internal_noalloc (SerialSource *source, SerialVersion0SourceInfo* );
gboolean serialize_version0sourceinfo                    (SerialSink *sink, const guint8 md5[16], const guint8 real_md5[16], guint32 length);
gboolean serialize_version0sourceinfo_obj                (SerialSink *sink, const SerialVersion0SourceInfo* obj);
gboolean serialize_version0sourceinfo_internal           (SerialSink *sink, const guint8 md5[16], const guint8 real_md5[16], guint32 length);
gboolean serialize_version0sourceinfo_obj_internal (SerialSink *sink, SerialVersion0SourceInfo* obj);
guint    serializeio_count_version0sourceinfo            (const guint8 md5[16], const guint8 real_md5[16], guint32 length);
guint    serializeio_count_version0sourceinfo_obj        (SerialVersion0SourceInfo const* obj);

/* Version0Control Structure
 */

struct _SerialVersion0Control {
  gboolean normalized;
  guint32 data_len;
  SerialVersion0SourceInfo to_info;
  guint32 source_info_len;
  SerialVersion0SourceInfo** source_info;
  guint32 inst_len;
  SerialVersion0Instruction* inst;
  GArray    *inst_array;
  GPtrArray *source_info_array;
};

void     serializeio_print_version0control_obj        (SerialVersion0Control* obj, guint indent_spaces);

gboolean unserialize_version0control                  (SerialSource *source, SerialVersion0Control**);
gboolean unserialize_version0control_internal         (SerialSource *source, SerialVersion0Control** );
gboolean unserialize_version0control_internal_noalloc (SerialSource *source, SerialVersion0Control* );
gboolean serialize_version0control                    (SerialSink *sink, gboolean normalized, guint32 data_len, SerialVersion0SourceInfo const* to_info, guint32 source_info_len, SerialVersion0SourceInfo* const* source_info, guint32 inst_len, SerialVersion0Instruction const* inst);
gboolean serialize_version0control_obj                (SerialSink *sink, const SerialVersion0Control* obj);
gboolean serialize_version0control_internal           (SerialSink *sink, gboolean normalized, guint32 data_len, SerialVersion0SourceInfo const* to_info, guint32 source_info_len, SerialVersion0SourceInfo* const* source_info, guint32 inst_len, SerialVersion0Instruction const* inst);
gboolean serialize_version0control_obj_internal (SerialSink *sink, SerialVersion0Control* obj);
guint    serializeio_count_version0control            (gboolean normalized, guint32 data_len, SerialVersion0SourceInfo const* to_info, guint32 source_info_len, SerialVersion0SourceInfo* const* source_info, guint32 inst_len, SerialVersion0Instruction const* inst);
guint    serializeio_count_version0control_obj        (SerialVersion0Control const* obj);

/* Version0Instruction Structure
 */

struct _SerialVersion0Instruction {
  guint32 offset;
  guint32 length;
  guint8 type;
  guint8 index;
};

void     serializeio_print_version0instruction_obj        (SerialVersion0Instruction* obj, guint indent_spaces);

gboolean unserialize_version0instruction                  (SerialSource *source, SerialVersion0Instruction**);
gboolean unserialize_version0instruction_internal         (SerialSource *source, SerialVersion0Instruction** );
gboolean unserialize_version0instruction_internal_noalloc (SerialSource *source, SerialVersion0Instruction* );
gboolean serialize_version0instruction                    (SerialSink *sink, guint32 offset, guint32 length);
gboolean serialize_version0instruction_obj                (SerialSink *sink, const SerialVersion0Instruction* obj);
gboolean serialize_version0instruction_internal           (SerialSink *sink, guint32 offset, guint32 length);
gboolean serialize_version0instruction_obj_internal (SerialSink *sink, SerialVersion0Instruction* obj);
guint    serializeio_count_version0instruction            (guint32 offset, guint32 length);
guint    serializeio_count_version0instruction_obj        (SerialVersion0Instruction const* obj);

void xd_generate_void_event_internal (XdVoidEventCode code, const char* srcfile, gint srcline);
#define xd_generate_void_event(ecode) xd_generate_void_event_internal((ecode),__FILE__,__LINE__)

extern const XdVoidEventCode EC_XdTooFewSources;
#define EC_XdTooFewSourcesValue ((0<<EDSIO_LIBRARY_OFFSET_BITS)+3)

extern const XdVoidEventCode EC_XdTooManySources;
#define EC_XdTooManySourcesValue ((1<<EDSIO_LIBRARY_OFFSET_BITS)+3)

void xd_generate_int_event_internal (XdIntEventCode code, const char* srcfile, gint srcline, int index);
#define xd_generate_int_event(ecode, index) xd_generate_int_event_internal((ecode),__FILE__,__LINE__, (index))

extern const XdIntEventCode EC_XdOutOfRangeSourceIndex;
#define EC_XdOutOfRangeSourceIndexValue ((2<<EDSIO_LIBRARY_OFFSET_BITS)+3)

extern const XdVoidEventCode EC_XdInvalidControl;
#define EC_XdInvalidControlValue ((3<<EDSIO_LIBRARY_OFFSET_BITS)+3)

extern const XdVoidEventCode EC_XdInvalidRsyncCache;
#define EC_XdInvalidRsyncCacheValue ((4<<EDSIO_LIBRARY_OFFSET_BITS)+3)

extern const XdVoidEventCode EC_XdIncompatibleDelta;
#define EC_XdIncompatibleDeltaValue ((5<<EDSIO_LIBRARY_OFFSET_BITS)+3)

void xd_generate_handlestringstring_event_internal (XdHandleStringStringEventCode code, const char* srcfile, gint srcline, FileHandle* stream, const char* expected, const char* received);
#define xd_generate_handlestringstring_event(ecode, stream, expected, received) xd_generate_handlestringstring_event_internal((ecode),__FILE__,__LINE__, (stream), (expected), (received))

extern const XdHandleStringStringEventCode EC_XdStreamChecksumFailed;
#define EC_XdStreamChecksumFailedValue ((6<<EDSIO_LIBRARY_OFFSET_BITS)+3)

void xd_generate_handleintint_event_internal (XdHandleIntIntEventCode code, const char* srcfile, gint srcline, FileHandle* stream, int expected, int received);
#define xd_generate_handleintint_event(ecode, stream, expected, received) xd_generate_handleintint_event_internal((ecode),__FILE__,__LINE__, (stream), (expected), (received))

extern const XdHandleIntIntEventCode EC_XdStreamLengthFailed;
#define EC_XdStreamLengthFailedValue ((7<<EDSIO_LIBRARY_OFFSET_BITS)+3)

void xd_generate_string_event_internal (XdStringEventCode code, const char* srcfile, gint srcline, const char* version);
#define xd_generate_string_event(ecode, version) xd_generate_string_event_internal((ecode),__FILE__,__LINE__, (version))

extern const XdStringEventCode EC_XdBackwardCompatibilityMode;
#define EC_XdBackwardCompatibilityModeValue ((8<<EDSIO_LIBRARY_OFFSET_BITS)+3)

#ifdef __cplusplus
}
#endif

#endif /* _XD_EDSIO_H_ */

