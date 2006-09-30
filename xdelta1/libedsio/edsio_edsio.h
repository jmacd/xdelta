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

#include "edsio.h"

#ifndef _EDSIO_EDSIO_H_
#define _EDSIO_EDSIO_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Initialize this library. */

gboolean edsio_edsio_init (void);

/* Types defined here. */

/* EdsioStringStringStringEventCode.
 */

typedef struct _EdsioStringStringStringEventCode EdsioStringStringStringEventCode;
struct _EdsioStringStringStringEventCode { gint code; };

typedef struct _EdsioStringStringStringEvent EdsioStringStringStringEvent;
struct _EdsioStringStringStringEvent { gint code; const char* srcfile; guint srcline; const char* name; const char* recv; const char* expect; };

/* EdsioIntEventCode.
 */

typedef struct _EdsioIntEventCode EdsioIntEventCode;
struct _EdsioIntEventCode { gint code; };

typedef struct _EdsioIntEvent EdsioIntEvent;
struct _EdsioIntEvent { gint code; const char* srcfile; guint srcline; int num; };

/* EdsioStringEventCode.
 */

typedef struct _EdsioStringEventCode EdsioStringEventCode;
struct _EdsioStringEventCode { gint code; };

typedef struct _EdsioStringEvent EdsioStringEvent;
struct _EdsioStringEvent { gint code; const char* srcfile; guint srcline; const char* name; };

/* EdsioStringStringEventCode.
 */

typedef struct _EdsioStringStringEventCode EdsioStringStringEventCode;
struct _EdsioStringStringEventCode { gint code; };

typedef struct _EdsioStringStringEvent EdsioStringStringEvent;
struct _EdsioStringStringEvent { gint code; const char* srcfile; guint srcline; const char* msg; const char* arg; };

/* EdsioSourceEventCode.
 */

typedef struct _EdsioSourceEventCode EdsioSourceEventCode;
struct _EdsioSourceEventCode { gint code; };

typedef struct _EdsioSourceEvent EdsioSourceEvent;
struct _EdsioSourceEvent { gint code; const char* srcfile; guint srcline; SerialSource* source; };

/* EdsioVoidEventCode.
 */

typedef struct _EdsioVoidEventCode EdsioVoidEventCode;
struct _EdsioVoidEventCode { gint code; };

typedef struct _EdsioVoidEvent EdsioVoidEvent;
struct _EdsioVoidEvent { gint code; const char* srcfile; guint srcline; };

/* EdsioIntIntEventCode.
 */

typedef struct _EdsioIntIntEventCode EdsioIntIntEventCode;
struct _EdsioIntIntEventCode { gint code; };

typedef struct _EdsioIntIntEvent EdsioIntIntEvent;
struct _EdsioIntIntEvent { gint code; const char* srcfile; guint srcline; int library; int number; };

/* EdsioErrnoEventCode.
 */

typedef struct _EdsioErrnoEventCode EdsioErrnoEventCode;
struct _EdsioErrnoEventCode { gint code; };

typedef struct _EdsioErrnoEvent EdsioErrnoEvent;
struct _EdsioErrnoEvent { gint code; const char* srcfile; guint srcline; gint ev_errno; };

typedef struct _SerialGenericTime SerialGenericTime;
typedef struct _SerialEdsioString SerialEdsioString;
typedef struct _SerialEdsioBytes SerialEdsioBytes;
typedef struct _SerialEdsioUint SerialEdsioUint;
/* Functions declared here. */

/* Property definitions */

/* Property get/set for PropTest/EdsioUint
 */

typedef struct _EdsioPropTestEdsioUintProperty EdsioPropTestEdsioUintProperty;
struct _EdsioPropTestEdsioUintProperty { guint32 code; };

gboolean edsio_new_proptest_edsiouint_property (const char* name, guint32 flags, EdsioPropTestEdsioUintProperty* prop);
gboolean proptest_get_edsiouint (PropTest* obj, EdsioPropTestEdsioUintProperty prop, SerialEdsioUint** arg);
gboolean proptest_set_edsiouint (PropTest* obj, EdsioPropTestEdsioUintProperty prop, SerialEdsioUint* arg);
gboolean proptest_unset_edsiouint (PropTest* obj, EdsioPropTestEdsioUintProperty prop);
gboolean proptest_isset_edsiouint (PropTest* obj, EdsioPropTestEdsioUintProperty prop);

/* Property get/set for PropTest/string
 */

typedef struct _EdsioPropTestStringProperty EdsioPropTestStringProperty;
struct _EdsioPropTestStringProperty { guint32 code; };

gboolean edsio_new_proptest_string_property (const char* name, guint32 flags, EdsioPropTestStringProperty* prop);
gboolean proptest_get_string (PropTest* obj, EdsioPropTestStringProperty prop, const gchar** arg);
gboolean proptest_set_string (PropTest* obj, EdsioPropTestStringProperty prop, const gchar* arg);
gboolean proptest_unset_string (PropTest* obj, EdsioPropTestStringProperty prop);
gboolean proptest_isset_string (PropTest* obj, EdsioPropTestStringProperty prop);

/* Property get/set for PropTest/bytes
 */

typedef struct _EdsioPropTestBytesProperty EdsioPropTestBytesProperty;
struct _EdsioPropTestBytesProperty { guint32 code; };

gboolean edsio_new_proptest_bytes_property (const char* name, guint32 flags, EdsioPropTestBytesProperty* prop);
gboolean proptest_get_bytes (PropTest* obj, EdsioPropTestBytesProperty prop, const guint8** arg, guint32* arg_len);
gboolean proptest_set_bytes (PropTest* obj, EdsioPropTestBytesProperty prop, const guint8* arg, guint32 arg_len);
gboolean proptest_unset_bytes (PropTest* obj, EdsioPropTestBytesProperty prop);
gboolean proptest_isset_bytes (PropTest* obj, EdsioPropTestBytesProperty prop);

/* Property get/set for PropTest/uint
 */

typedef struct _EdsioPropTestUintProperty EdsioPropTestUintProperty;
struct _EdsioPropTestUintProperty { guint32 code; };

gboolean edsio_new_proptest_uint_property (const char* name, guint32 flags, EdsioPropTestUintProperty* prop);
gboolean proptest_get_uint (PropTest* obj, EdsioPropTestUintProperty prop, guint32* arg);
gboolean proptest_set_uint (PropTest* obj, EdsioPropTestUintProperty prop, guint32 arg);
gboolean proptest_unset_uint (PropTest* obj, EdsioPropTestUintProperty prop);
gboolean proptest_isset_uint (PropTest* obj, EdsioPropTestUintProperty prop);

/* Serial Types */

enum _SerialEdsioType {

  ST_EdsioUint = (1<<(1+EDSIO_LIBRARY_OFFSET_BITS))+6, 
  ST_EdsioBytes = (1<<(2+EDSIO_LIBRARY_OFFSET_BITS))+6, 
  ST_EdsioString = (1<<(3+EDSIO_LIBRARY_OFFSET_BITS))+6, 
  ST_GenericTime = (1<<(4+EDSIO_LIBRARY_OFFSET_BITS))+6
};



/* EdsioUint Structure
 */

struct _SerialEdsioUint {
  guint32 val;
};

void     serializeio_print_edsiouint_obj        (SerialEdsioUint* obj, guint indent_spaces);

gboolean unserialize_edsiouint                  (SerialSource *source, SerialEdsioUint**);
gboolean unserialize_edsiouint_internal         (SerialSource *source, SerialEdsioUint** );
gboolean unserialize_edsiouint_internal_noalloc (SerialSource *source, SerialEdsioUint* );
gboolean serialize_edsiouint                    (SerialSink *sink, guint32 val);
gboolean serialize_edsiouint_obj                (SerialSink *sink, const SerialEdsioUint* obj);
gboolean serialize_edsiouint_internal           (SerialSink *sink, guint32 val);
gboolean serialize_edsiouint_obj_internal (SerialSink *sink, SerialEdsioUint* obj);
guint    serializeio_count_edsiouint            (guint32 val);
guint    serializeio_count_edsiouint_obj        (SerialEdsioUint const* obj);

/* EdsioBytes Structure
 */

struct _SerialEdsioBytes {
  guint32 val_len;
  const guint8* val;
};

void     serializeio_print_edsiobytes_obj        (SerialEdsioBytes* obj, guint indent_spaces);

gboolean unserialize_edsiobytes                  (SerialSource *source, SerialEdsioBytes**);
gboolean unserialize_edsiobytes_internal         (SerialSource *source, SerialEdsioBytes** );
gboolean unserialize_edsiobytes_internal_noalloc (SerialSource *source, SerialEdsioBytes* );
gboolean serialize_edsiobytes                    (SerialSink *sink, guint32 val_len, const guint8* val);
gboolean serialize_edsiobytes_obj                (SerialSink *sink, const SerialEdsioBytes* obj);
gboolean serialize_edsiobytes_internal           (SerialSink *sink, guint32 val_len, const guint8* val);
gboolean serialize_edsiobytes_obj_internal (SerialSink *sink, SerialEdsioBytes* obj);
guint    serializeio_count_edsiobytes            (guint32 val_len, const guint8* val);
guint    serializeio_count_edsiobytes_obj        (SerialEdsioBytes const* obj);

/* EdsioString Structure
 */

struct _SerialEdsioString {
  const gchar* val;
};

void     serializeio_print_edsiostring_obj        (SerialEdsioString* obj, guint indent_spaces);

gboolean unserialize_edsiostring                  (SerialSource *source, SerialEdsioString**);
gboolean unserialize_edsiostring_internal         (SerialSource *source, SerialEdsioString** );
gboolean unserialize_edsiostring_internal_noalloc (SerialSource *source, SerialEdsioString* );
gboolean serialize_edsiostring                    (SerialSink *sink, const gchar* val);
gboolean serialize_edsiostring_obj                (SerialSink *sink, const SerialEdsioString* obj);
gboolean serialize_edsiostring_internal           (SerialSink *sink, const gchar* val);
gboolean serialize_edsiostring_obj_internal (SerialSink *sink, SerialEdsioString* obj);
guint    serializeio_count_edsiostring            (const gchar* val);
guint    serializeio_count_edsiostring_obj        (SerialEdsioString const* obj);

/* GenericTime Structure
 */

struct _SerialGenericTime {
  guint32 seconds;
  guint32 nanos;
};

void     serializeio_print_generictime_obj        (SerialGenericTime* obj, guint indent_spaces);

gboolean unserialize_generictime                  (SerialSource *source, SerialGenericTime**);
gboolean unserialize_generictime_internal         (SerialSource *source, SerialGenericTime** );
gboolean unserialize_generictime_internal_noalloc (SerialSource *source, SerialGenericTime* );
gboolean serialize_generictime                    (SerialSink *sink, guint32 seconds, guint32 nanos);
gboolean serialize_generictime_obj                (SerialSink *sink, const SerialGenericTime* obj);
gboolean serialize_generictime_internal           (SerialSink *sink, guint32 seconds, guint32 nanos);
gboolean serialize_generictime_obj_internal (SerialSink *sink, SerialGenericTime* obj);
guint    serializeio_count_generictime            (guint32 seconds, guint32 nanos);
guint    serializeio_count_generictime_obj        (SerialGenericTime const* obj);

void edsio_generate_errno_event_internal (EdsioErrnoEventCode code, const char* srcfile, gint srcline);
#define edsio_generate_errno_event(ecode) edsio_generate_errno_event_internal((ecode),__FILE__,__LINE__)

extern const EdsioErrnoEventCode EC_EdsioGetTimeOfDayFailure;
#define EC_EdsioGetTimeOfDayFailureValue ((0<<EDSIO_LIBRARY_OFFSET_BITS)+6)

extern const EdsioErrnoEventCode EC_EdsioTimeFailure;
#define EC_EdsioTimeFailureValue ((1<<EDSIO_LIBRARY_OFFSET_BITS)+6)

void edsio_generate_intint_event_internal (EdsioIntIntEventCode code, const char* srcfile, gint srcline, int library, int number);
#define edsio_generate_intint_event(ecode, library, number) edsio_generate_intint_event_internal((ecode),__FILE__,__LINE__, (library), (number))

extern const EdsioIntIntEventCode EC_EdsioUnregisteredType;
#define EC_EdsioUnregisteredTypeValue ((2<<EDSIO_LIBRARY_OFFSET_BITS)+6)

extern const EdsioIntIntEventCode EC_EdsioUnexpectedLibraryType;
#define EC_EdsioUnexpectedLibraryTypeValue ((3<<EDSIO_LIBRARY_OFFSET_BITS)+6)

void edsio_generate_void_event_internal (EdsioVoidEventCode code, const char* srcfile, gint srcline);
#define edsio_generate_void_event(ecode) edsio_generate_void_event_internal((ecode),__FILE__,__LINE__)

extern const EdsioVoidEventCode EC_EdsioUnexpectedType;
#define EC_EdsioUnexpectedTypeValue ((4<<EDSIO_LIBRARY_OFFSET_BITS)+6)

extern const EdsioVoidEventCode EC_EdsioOutputBufferShort;
#define EC_EdsioOutputBufferShortValue ((5<<EDSIO_LIBRARY_OFFSET_BITS)+6)

extern const EdsioVoidEventCode EC_EdsioInvalidBase64Encoding;
#define EC_EdsioInvalidBase64EncodingValue ((6<<EDSIO_LIBRARY_OFFSET_BITS)+6)

extern const EdsioVoidEventCode EC_EdsioMissingChecksum;
#define EC_EdsioMissingChecksumValue ((7<<EDSIO_LIBRARY_OFFSET_BITS)+6)

extern const EdsioVoidEventCode EC_EdsioInvalidChecksum;
#define EC_EdsioInvalidChecksumValue ((8<<EDSIO_LIBRARY_OFFSET_BITS)+6)

void edsio_generate_source_event_internal (EdsioSourceEventCode code, const char* srcfile, gint srcline, SerialSource* source);
#define edsio_generate_source_event(ecode, source) edsio_generate_source_event_internal((ecode),__FILE__,__LINE__, (source))

extern const EdsioSourceEventCode EC_EdsioSourceEof;
#define EC_EdsioSourceEofValue ((9<<EDSIO_LIBRARY_OFFSET_BITS)+6)

extern const EdsioSourceEventCode EC_EdsioIncorrectAllocation;
#define EC_EdsioIncorrectAllocationValue ((10<<EDSIO_LIBRARY_OFFSET_BITS)+6)

void edsio_generate_stringstring_event_internal (EdsioStringStringEventCode code, const char* srcfile, gint srcline, const char* msg, const char* arg);
#define edsio_generate_stringstring_event(ecode, msg, arg) edsio_generate_stringstring_event_internal((ecode),__FILE__,__LINE__, (msg), (arg))

extern const EdsioStringStringEventCode EC_EdsioInvalidIntegerString;
#define EC_EdsioInvalidIntegerStringValue ((11<<EDSIO_LIBRARY_OFFSET_BITS)+6)

extern const EdsioStringStringEventCode EC_EdsioIntegerOutOfRange;
#define EC_EdsioIntegerOutOfRangeValue ((12<<EDSIO_LIBRARY_OFFSET_BITS)+6)

extern const EdsioStringStringEventCode EC_EdsioInvalidIntegerSign;
#define EC_EdsioInvalidIntegerSignValue ((13<<EDSIO_LIBRARY_OFFSET_BITS)+6)

void edsio_generate_string_event_internal (EdsioStringEventCode code, const char* srcfile, gint srcline, const char* name);
#define edsio_generate_string_event(ecode, name) edsio_generate_string_event_internal((ecode),__FILE__,__LINE__, (name))

extern const EdsioStringEventCode EC_EdsioDuplicatePropertyTypeRegistered;
#define EC_EdsioDuplicatePropertyTypeRegisteredValue ((14<<EDSIO_LIBRARY_OFFSET_BITS)+6)

extern const EdsioStringEventCode EC_EdsioDuplicateHostTypeRegistered;
#define EC_EdsioDuplicateHostTypeRegisteredValue ((15<<EDSIO_LIBRARY_OFFSET_BITS)+6)

extern const EdsioStringEventCode EC_EdsioDuplicatePropertyNameRegistered;
#define EC_EdsioDuplicatePropertyNameRegisteredValue ((16<<EDSIO_LIBRARY_OFFSET_BITS)+6)

void edsio_generate_int_event_internal (EdsioIntEventCode code, const char* srcfile, gint srcline, int num);
#define edsio_generate_int_event(ecode, num) edsio_generate_int_event_internal((ecode),__FILE__,__LINE__, (num))

extern const EdsioIntEventCode EC_EdsioNoSuchProperty;
#define EC_EdsioNoSuchPropertyValue ((17<<EDSIO_LIBRARY_OFFSET_BITS)+6)

extern const EdsioStringEventCode EC_EdsioNoSuchPropertyType;
#define EC_EdsioNoSuchPropertyTypeValue ((18<<EDSIO_LIBRARY_OFFSET_BITS)+6)

extern const EdsioStringEventCode EC_EdsioNoSuchHostType;
#define EC_EdsioNoSuchHostTypeValue ((19<<EDSIO_LIBRARY_OFFSET_BITS)+6)

void edsio_generate_stringstringstring_event_internal (EdsioStringStringStringEventCode code, const char* srcfile, gint srcline, const char* name, const char* recv, const char* expect);
#define edsio_generate_stringstringstring_event(ecode, name, recv, expect) edsio_generate_stringstringstring_event_internal((ecode),__FILE__,__LINE__, (name), (recv), (expect))

extern const EdsioStringStringStringEventCode EC_EdsioWrongHostType;
#define EC_EdsioWrongHostTypeValue ((20<<EDSIO_LIBRARY_OFFSET_BITS)+6)

extern const EdsioStringStringStringEventCode EC_EdsioWrongDataType;
#define EC_EdsioWrongDataTypeValue ((21<<EDSIO_LIBRARY_OFFSET_BITS)+6)

extern const EdsioStringEventCode EC_EdsioPropertyNotSet;
#define EC_EdsioPropertyNotSetValue ((22<<EDSIO_LIBRARY_OFFSET_BITS)+6)

extern const EdsioStringStringEventCode EC_EdsioPersistenceUnavailable;
#define EC_EdsioPersistenceUnavailableValue ((23<<EDSIO_LIBRARY_OFFSET_BITS)+6)

extern const EdsioVoidEventCode EC_EdsioInvalidStreamChecksum;
#define EC_EdsioInvalidStreamChecksumValue ((24<<EDSIO_LIBRARY_OFFSET_BITS)+6)

extern const EdsioStringStringEventCode EC_EdsioInvalidHexDigit;
#define EC_EdsioInvalidHexDigitValue ((25<<EDSIO_LIBRARY_OFFSET_BITS)+6)

extern const EdsioStringEventCode EC_EdsioMD5StringShort;
#define EC_EdsioMD5StringShortValue ((26<<EDSIO_LIBRARY_OFFSET_BITS)+6)

extern const EdsioStringEventCode EC_EdsioMD5StringLong;
#define EC_EdsioMD5StringLongValue ((27<<EDSIO_LIBRARY_OFFSET_BITS)+6)

extern const EdsioIntEventCode EC_EdsioUnregisteredLibrary;
#define EC_EdsioUnregisteredLibraryValue ((28<<EDSIO_LIBRARY_OFFSET_BITS)+6)

extern const EdsioStringStringEventCode EC_EdsioGModuleError;
#define EC_EdsioGModuleErrorValue ((29<<EDSIO_LIBRARY_OFFSET_BITS)+6)

#ifdef __cplusplus
}
#endif

#endif /* _EDSIO_EDSIO_H_ */

