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

#include <errno.h>

/* Declarations. */

static const char* Edsio_StringStringString_event_field_to_string (GenericEvent* ev, gint field);
static const char* Edsio_Int_event_field_to_string (GenericEvent* ev, gint field);
static const char* Edsio_String_event_field_to_string (GenericEvent* ev, gint field);
static const char* Edsio_StringString_event_field_to_string (GenericEvent* ev, gint field);
static const char* Edsio_Source_event_field_to_string (GenericEvent* ev, gint field);
static const char* Edsio_IntInt_event_field_to_string (GenericEvent* ev, gint field);
static const char* Edsio_Errno_event_field_to_string (GenericEvent* ev, gint field);
static void print_spaces (guint n) { int i; for (i = 0; i < n; i += 1) g_print (" "); }


/* initialize this library. */

gboolean
edsio_edsio_init (void)
{
  static gboolean once = FALSE;
  static gboolean result = FALSE;
  if (once) return result;
  once = TRUE;
  eventdelivery_initialize_event_def (EC_EdsioGModuleErrorValue, EL_Error, EF_None, "GModuleError", "GModule: ${0}: ${1}", & Edsio_StringString_event_field_to_string);
  eventdelivery_initialize_event_def (EC_EdsioUnregisteredLibraryValue, EL_Error, EF_None, "UnregisteredLibrary", "Unregistered library: ${0}", & Edsio_Int_event_field_to_string);
  eventdelivery_initialize_event_def (EC_EdsioMD5StringLongValue, EL_Error, EF_None, "MD5StringLong", "MD5 string too long: ${0}", & Edsio_String_event_field_to_string);
  eventdelivery_initialize_event_def (EC_EdsioMD5StringShortValue, EL_Error, EF_None, "MD5StringShort", "MD5 string too short: ${0}", & Edsio_String_event_field_to_string);
  eventdelivery_initialize_event_def (EC_EdsioInvalidHexDigitValue, EL_Error, EF_None, "InvalidHexDigit", "Invalid hex digit ${0} in context: ${1}", & Edsio_StringString_event_field_to_string);
  eventdelivery_initialize_event_def (EC_EdsioInvalidStreamChecksumValue, EL_Error, EF_None, "InvalidStreamChecksum", "Incorrect stream checksum", NULL);
  eventdelivery_initialize_event_def (EC_EdsioPersistenceUnavailableValue, EL_Error, EF_None, "PersistenceUnavailable", "Persistence is unavailable in host ${1} for property ${0}", & Edsio_StringString_event_field_to_string);
  eventdelivery_initialize_event_def (EC_EdsioPropertyNotSetValue, EL_Error, EF_None, "PropertyNotSet", "${0} property not set", & Edsio_String_event_field_to_string);
  eventdelivery_initialize_event_def (EC_EdsioWrongDataTypeValue, EL_Error, EF_None, "WrongDataType", "Wrong property data type: received ${1}, expected ${2}", & Edsio_StringStringString_event_field_to_string);
  eventdelivery_initialize_event_def (EC_EdsioWrongHostTypeValue, EL_Error, EF_None, "WrongHostType", "Wrong property host type: received ${1}, expected ${2}", & Edsio_StringStringString_event_field_to_string);
  eventdelivery_initialize_event_def (EC_EdsioNoSuchHostTypeValue, EL_Error, EF_None, "NoSuchHostType", "Unregistered host type: ${0}", & Edsio_String_event_field_to_string);
  eventdelivery_initialize_event_def (EC_EdsioNoSuchPropertyTypeValue, EL_Error, EF_None, "NoSuchPropertyType", "Unregistered property type: ${0}", & Edsio_String_event_field_to_string);
  eventdelivery_initialize_event_def (EC_EdsioNoSuchPropertyValue, EL_Error, EF_None, "NoSuchProperty", "Unregistered property: ${0}", & Edsio_Int_event_field_to_string);
  eventdelivery_initialize_event_def (EC_EdsioDuplicatePropertyNameRegisteredValue, EL_Warning, EF_None, "DuplicatePropertyNameRegistered", "Property name registered twice (ignored): ${0}", & Edsio_String_event_field_to_string);
  eventdelivery_initialize_event_def (EC_EdsioDuplicateHostTypeRegisteredValue, EL_Error, EF_None, "DuplicateHostTypeRegistered", "Property host registered twice: ${0}", & Edsio_String_event_field_to_string);
  eventdelivery_initialize_event_def (EC_EdsioDuplicatePropertyTypeRegisteredValue, EL_Error, EF_None, "DuplicatePropertyTypeRegistered", "Property type registered twice: ${0}", & Edsio_String_event_field_to_string);
  eventdelivery_initialize_event_def (EC_EdsioInvalidIntegerSignValue, EL_Error, EF_None, "InvalidIntegerSign", "${0}: expected an unsigned integer: ${1}", & Edsio_StringString_event_field_to_string);
  eventdelivery_initialize_event_def (EC_EdsioIntegerOutOfRangeValue, EL_Error, EF_None, "IntegerOutOfRange", "${0}: integer out of range: ${1}", & Edsio_StringString_event_field_to_string);
  eventdelivery_initialize_event_def (EC_EdsioInvalidIntegerStringValue, EL_Error, EF_None, "InvalidIntegerString", "${0}: not an integer: ${1}", & Edsio_StringString_event_field_to_string);
  eventdelivery_initialize_event_def (EC_EdsioIncorrectAllocationValue, EL_Error, EF_None, "IncorrectAllocation", "${0}: Incorrect allocation", & Edsio_Source_event_field_to_string);
  eventdelivery_initialize_event_def (EC_EdsioSourceEofValue, EL_Error, EF_None, "SourceEof", "${0}: Unexpected EOF", & Edsio_Source_event_field_to_string);
  eventdelivery_initialize_event_def (EC_EdsioInvalidChecksumValue, EL_Error, EF_None, "InvalidChecksum", "Checksum verification failed", NULL);
  eventdelivery_initialize_event_def (EC_EdsioMissingChecksumValue, EL_Error, EF_None, "MissingChecksum", "Missing embedded checksum in base64 encoding", NULL);
  eventdelivery_initialize_event_def (EC_EdsioInvalidBase64EncodingValue, EL_Error, EF_None, "InvalidBase64Encoding", "Invalid base64 encoding", NULL);
  eventdelivery_initialize_event_def (EC_EdsioOutputBufferShortValue, EL_Error, EF_None, "OutputBufferShort", "Output buffer is too short", NULL);
  eventdelivery_initialize_event_def (EC_EdsioUnexpectedTypeValue, EL_Error, EF_None, "UnexpectedType", "Unexpected serial type", NULL);
  eventdelivery_initialize_event_def (EC_EdsioUnexpectedLibraryTypeValue, EL_Error, EF_None, "UnexpectedLibraryType", "Unexpected serial library type: expected ${0}, received ${1}", & Edsio_IntInt_event_field_to_string);
  eventdelivery_initialize_event_def (EC_EdsioUnregisteredTypeValue, EL_Error, EF_None, "UnregisteredType", "Unregistered serial type: library=${0} number=${1}", & Edsio_IntInt_event_field_to_string);
  eventdelivery_initialize_event_def (EC_EdsioTimeFailureValue, EL_Error, EF_None, "TimeFailure", "Time failed: ${0}", & Edsio_Errno_event_field_to_string);
  eventdelivery_initialize_event_def (EC_EdsioGetTimeOfDayFailureValue, EL_Error, EF_None, "GetTimeOfDayFailure", "Gettimeofday failed: ${0}", & Edsio_Errno_event_field_to_string);
  serializeio_initialize_type ("ST_GenericTime", ST_GenericTime, &unserialize_generictime_internal, &serialize_generictime_obj_internal, &serializeio_count_generictime_obj, &serializeio_print_generictime_obj);
  serializeio_initialize_type ("ST_EdsioString", ST_EdsioString, &unserialize_edsiostring_internal, &serialize_edsiostring_obj_internal, &serializeio_count_edsiostring_obj, &serializeio_print_edsiostring_obj);
  serializeio_initialize_type ("ST_EdsioBytes", ST_EdsioBytes, &unserialize_edsiobytes_internal, &serialize_edsiobytes_obj_internal, &serializeio_count_edsiobytes_obj, &serializeio_print_edsiobytes_obj);
  serializeio_initialize_type ("ST_EdsioUint", ST_EdsioUint, &unserialize_edsiouint_internal, &serialize_edsiouint_obj_internal, &serializeio_count_edsiouint_obj, &serializeio_print_edsiouint_obj);
  edsio_initialize_host_type ("PropTest", (PropertyTableFunc) & edsio_proptest_property_table, (PersistSourceFunc) & edsio_persist_proptest_source, (PersistSinkFunc) & edsio_persist_proptest_sink, (PersistIssetFunc) & edsio_persist_proptest_isset, (PersistUnsetFunc) & edsio_persist_proptest_unset);
  edsio_initialize_property_type ("EdsioUint", & edsio_property_vptr_free, & edsio_property_vptr_getter, & edsio_property_vptr_setter, serialize_edsiouint_obj, unserialize_edsiouint);
  edsio_initialize_property_type ("string", & edsio_property_string_free, & edsio_property_string_getter, & edsio_property_string_setter, serialize_string_obj, unserialize_string);
  edsio_initialize_property_type ("bytes", & edsio_property_bytes_free, & edsio_property_bytes_getter, & edsio_property_bytes_setter, serialize_bytes_obj, unserialize_bytes);
  edsio_initialize_property_type ("uint", & edsio_property_uint_free, & edsio_property_uint_getter, & edsio_property_uint_setter, serialize_uint_obj, unserialize_uint);
  edsio_library_register (6, "edsio");
  result = TRUE;
  return TRUE;
};

gboolean edsio_new_proptest_edsiouint_property (const char* name, guint32 flags, EdsioPropTestEdsioUintProperty* prop)
{
  return edsio_new_property (name, "PropTest", "EdsioUint", flags, (EdsioGenericProperty*) prop);
}

gboolean
proptest_get_edsiouint (PropTest* obj, EdsioPropTestEdsioUintProperty prop, SerialEdsioUint** arg)
{
  EdsioProperty* ep;
  g_return_val_if_fail (obj, FALSE);
  return (* edsio_property_getter ("PropTest", "EdsioUint", prop.code, & ep)) (obj, ep, arg);
}

gboolean
proptest_set_edsiouint (PropTest* obj, EdsioPropTestEdsioUintProperty prop, SerialEdsioUint* arg)
{
  EdsioProperty* ep;
  g_return_val_if_fail (obj, FALSE);
  return (* edsio_property_setter ("PropTest", "EdsioUint", prop.code, & ep)) (obj, ep, arg);
}

gboolean
proptest_unset_edsiouint (PropTest* obj, EdsioPropTestEdsioUintProperty prop)
{
  g_return_val_if_fail (obj, FALSE);
  return edsio_property_unset ("PropTest", "EdsioUint", prop.code, obj);
}

gboolean
proptest_isset_edsiouint (PropTest* obj, EdsioPropTestEdsioUintProperty prop)
{
  g_return_val_if_fail (obj, FALSE);
  return edsio_property_isset ("PropTest", "EdsioUint", prop.code, obj);
}

gboolean edsio_new_proptest_string_property (const char* name, guint32 flags, EdsioPropTestStringProperty* prop)
{
  return edsio_new_property (name, "PropTest", "string", flags, (EdsioGenericProperty*) prop);
}

gboolean
proptest_get_string (PropTest* obj, EdsioPropTestStringProperty prop, const gchar** arg)
{
  EdsioProperty* ep;
  g_return_val_if_fail (obj, FALSE);
  return (* edsio_property_getter ("PropTest", "string", prop.code, & ep)) (obj, ep, arg);
}

gboolean
proptest_set_string (PropTest* obj, EdsioPropTestStringProperty prop, const gchar* arg)
{
  EdsioProperty* ep;
  g_return_val_if_fail (obj, FALSE);
  return (* edsio_property_setter ("PropTest", "string", prop.code, & ep)) (obj, ep, arg);
}

gboolean
proptest_unset_string (PropTest* obj, EdsioPropTestStringProperty prop)
{
  g_return_val_if_fail (obj, FALSE);
  return edsio_property_unset ("PropTest", "string", prop.code, obj);
}

gboolean
proptest_isset_string (PropTest* obj, EdsioPropTestStringProperty prop)
{
  g_return_val_if_fail (obj, FALSE);
  return edsio_property_isset ("PropTest", "string", prop.code, obj);
}

gboolean edsio_new_proptest_bytes_property (const char* name, guint32 flags, EdsioPropTestBytesProperty* prop)
{
  return edsio_new_property (name, "PropTest", "bytes", flags, (EdsioGenericProperty*) prop);
}

gboolean
proptest_get_bytes (PropTest* obj, EdsioPropTestBytesProperty prop, const guint8** arg, guint32* arg_len)
{
  EdsioProperty* ep;
  g_return_val_if_fail (obj, FALSE);
  return (* edsio_property_getter ("PropTest", "bytes", prop.code, & ep)) (obj, ep, arg, arg_len);
}

gboolean
proptest_set_bytes (PropTest* obj, EdsioPropTestBytesProperty prop, const guint8* arg, guint32 arg_len)
{
  EdsioProperty* ep;
  g_return_val_if_fail (obj, FALSE);
  return (* edsio_property_setter ("PropTest", "bytes", prop.code, & ep)) (obj, ep, arg, arg_len);
}

gboolean
proptest_unset_bytes (PropTest* obj, EdsioPropTestBytesProperty prop)
{
  g_return_val_if_fail (obj, FALSE);
  return edsio_property_unset ("PropTest", "bytes", prop.code, obj);
}

gboolean
proptest_isset_bytes (PropTest* obj, EdsioPropTestBytesProperty prop)
{
  g_return_val_if_fail (obj, FALSE);
  return edsio_property_isset ("PropTest", "bytes", prop.code, obj);
}

gboolean edsio_new_proptest_uint_property (const char* name, guint32 flags, EdsioPropTestUintProperty* prop)
{
  return edsio_new_property (name, "PropTest", "uint", flags, (EdsioGenericProperty*) prop);
}

gboolean
proptest_get_uint (PropTest* obj, EdsioPropTestUintProperty prop, guint32* arg)
{
  EdsioProperty* ep;
  g_return_val_if_fail (obj, FALSE);
  return (* edsio_property_getter ("PropTest", "uint", prop.code, & ep)) (obj, ep, arg);
}

gboolean
proptest_set_uint (PropTest* obj, EdsioPropTestUintProperty prop, guint32 arg)
{
  EdsioProperty* ep;
  g_return_val_if_fail (obj, FALSE);
  return (* edsio_property_setter ("PropTest", "uint", prop.code, & ep)) (obj, ep, arg);
}

gboolean
proptest_unset_uint (PropTest* obj, EdsioPropTestUintProperty prop)
{
  g_return_val_if_fail (obj, FALSE);
  return edsio_property_unset ("PropTest", "uint", prop.code, obj);
}

gboolean
proptest_isset_uint (PropTest* obj, EdsioPropTestUintProperty prop)
{
  g_return_val_if_fail (obj, FALSE);
  return edsio_property_isset ("PropTest", "uint", prop.code, obj);
}

/* EdsioUint Count
 */

guint
serializeio_count_edsiouint (guint32 val) {
  guint size = sizeof (SerialEdsioUint);
  ALIGN_8 (size);
  ALIGN_8 (size);
  return size;
}

guint
serializeio_count_edsiouint_obj (SerialEdsioUint const* obj) {
  return serializeio_count_edsiouint (obj->val);
}

/* EdsioUint Print
 */

void
serializeio_print_edsiouint_obj (SerialEdsioUint* obj, guint indent_spaces) {
  print_spaces (indent_spaces);
  g_print ("[ST_EdsioUint]\n");
  print_spaces (indent_spaces);
  g_print ("val = ");
  g_print ("%d\n", obj->val);
}

/* EdsioUint Serialize
 */

gboolean
serialize_edsiouint_internal (SerialSink *sink, guint32 val)
{
  if (! (* sink->next_uint) (sink, val)) goto bail;
  return TRUE;
bail:
  return FALSE;
}

gboolean
serialize_edsiouint_obj_internal (SerialSink *sink, SerialEdsioUint* obj)
{
  return serialize_edsiouint_internal (sink, obj->val);
}

gboolean
serialize_edsiouint (SerialSink *sink, guint32 val)
{
  if (! (* sink->sink_type) (sink, ST_EdsioUint, serializeio_count_edsiouint (val), TRUE)) goto bail;
  if (! serialize_edsiouint_internal (sink, val)) goto bail;
  if (sink->sink_quantum && ! sink->sink_quantum (sink)) goto bail;
  return TRUE;
bail:
  return FALSE;
}

gboolean
serialize_edsiouint_obj (SerialSink *sink, const SerialEdsioUint* obj) {

  return serialize_edsiouint (sink, obj->val);
}

/* EdsioUint Unserialize
 */

gboolean
unserialize_edsiouint_internal_noalloc (SerialSource *source, SerialEdsioUint* result)
{
  if (! (* source->next_uint) (source, &result->val)) goto bail;
  return TRUE;
bail:
  return FALSE;
}

gboolean
unserialize_edsiouint_internal (SerialSource *source, SerialEdsioUint** result)
{
  SerialEdsioUint* unser;
  (*result) = NULL;
  unser = serializeio_source_alloc (source, sizeof (SerialEdsioUint));
  if (! unser) goto bail;
  if (! unserialize_edsiouint_internal_noalloc (source, unser)) goto bail;
  (*result) = unser;
  return TRUE;
bail:
  return FALSE;
}

gboolean
unserialize_edsiouint (SerialSource *source, SerialEdsioUint** result)
{
  if ( (* source->source_type) (source, TRUE) != ST_EdsioUint) goto bail;
  if (! unserialize_edsiouint_internal (source, result)) goto bail;
  if (! serializeio_source_object_received (source)) goto bail;
  return TRUE;
bail:
  return FALSE;
}

/* EdsioBytes Count
 */

guint
serializeio_count_edsiobytes (guint32 val_len, const guint8* val) {
  guint size = sizeof (SerialEdsioBytes);
  ALIGN_8 (size);
  size += val_len;
  ALIGN_8 (size);
  return size;
}

guint
serializeio_count_edsiobytes_obj (SerialEdsioBytes const* obj) {
  return serializeio_count_edsiobytes (obj->val_len, obj->val);
}

/* EdsioBytes Print
 */

void
serializeio_print_edsiobytes_obj (SerialEdsioBytes* obj, guint indent_spaces) {
  print_spaces (indent_spaces);
  g_print ("[ST_EdsioBytes]\n");
  print_spaces (indent_spaces);
  g_print ("val = ");
  serializeio_print_bytes (obj->val, obj->val_len);
}

/* EdsioBytes Serialize
 */

gboolean
serialize_edsiobytes_internal (SerialSink *sink, guint32 val_len, const guint8* val)
{
  if (! (* sink->next_bytes) (sink, val, val_len)) goto bail;
  return TRUE;
bail:
  return FALSE;
}

gboolean
serialize_edsiobytes_obj_internal (SerialSink *sink, SerialEdsioBytes* obj)
{
  return serialize_edsiobytes_internal (sink, obj->val_len, obj->val);
}

gboolean
serialize_edsiobytes (SerialSink *sink, guint32 val_len, const guint8* val)
{
  if (! (* sink->sink_type) (sink, ST_EdsioBytes, serializeio_count_edsiobytes (val_len, val), TRUE)) goto bail;
  if (! serialize_edsiobytes_internal (sink, val_len, val)) goto bail;
  if (sink->sink_quantum && ! sink->sink_quantum (sink)) goto bail;
  return TRUE;
bail:
  return FALSE;
}

gboolean
serialize_edsiobytes_obj (SerialSink *sink, const SerialEdsioBytes* obj) {

  return serialize_edsiobytes (sink, obj->val_len, obj->val);
}

/* EdsioBytes Unserialize
 */

gboolean
unserialize_edsiobytes_internal_noalloc (SerialSource *source, SerialEdsioBytes* result)
{
  if (! (* source->next_bytes) (source, &result->val, &result->val_len)) goto bail;
  return TRUE;
bail:
  return FALSE;
}

gboolean
unserialize_edsiobytes_internal (SerialSource *source, SerialEdsioBytes** result)
{
  SerialEdsioBytes* unser;
  (*result) = NULL;
  unser = serializeio_source_alloc (source, sizeof (SerialEdsioBytes));
  if (! unser) goto bail;
  if (! unserialize_edsiobytes_internal_noalloc (source, unser)) goto bail;
  (*result) = unser;
  return TRUE;
bail:
  return FALSE;
}

gboolean
unserialize_edsiobytes (SerialSource *source, SerialEdsioBytes** result)
{
  if ( (* source->source_type) (source, TRUE) != ST_EdsioBytes) goto bail;
  if (! unserialize_edsiobytes_internal (source, result)) goto bail;
  if (! serializeio_source_object_received (source)) goto bail;
  return TRUE;
bail:
  return FALSE;
}

/* EdsioString Count
 */

guint
serializeio_count_edsiostring (const gchar* val) {
  guint size = sizeof (SerialEdsioString);
  ALIGN_8 (size);
  size += strlen (val) + 1;
  ALIGN_8 (size);
  return size;
}

guint
serializeio_count_edsiostring_obj (SerialEdsioString const* obj) {
  return serializeio_count_edsiostring (obj->val);
}

/* EdsioString Print
 */

void
serializeio_print_edsiostring_obj (SerialEdsioString* obj, guint indent_spaces) {
  print_spaces (indent_spaces);
  g_print ("[ST_EdsioString]\n");
  print_spaces (indent_spaces);
  g_print ("val = ");
  g_print ("%s\n", obj->val);
}

/* EdsioString Serialize
 */

gboolean
serialize_edsiostring_internal (SerialSink *sink, const gchar* val)
{
  if (! (* sink->next_string) (sink, val)) goto bail;
  return TRUE;
bail:
  return FALSE;
}

gboolean
serialize_edsiostring_obj_internal (SerialSink *sink, SerialEdsioString* obj)
{
  return serialize_edsiostring_internal (sink, obj->val);
}

gboolean
serialize_edsiostring (SerialSink *sink, const gchar* val)
{
  if (! (* sink->sink_type) (sink, ST_EdsioString, serializeio_count_edsiostring (val), TRUE)) goto bail;
  if (! serialize_edsiostring_internal (sink, val)) goto bail;
  if (sink->sink_quantum && ! sink->sink_quantum (sink)) goto bail;
  return TRUE;
bail:
  return FALSE;
}

gboolean
serialize_edsiostring_obj (SerialSink *sink, const SerialEdsioString* obj) {

  return serialize_edsiostring (sink, obj->val);
}

/* EdsioString Unserialize
 */

gboolean
unserialize_edsiostring_internal_noalloc (SerialSource *source, SerialEdsioString* result)
{
  if (! (* source->next_string) (source, &result->val)) goto bail;
  return TRUE;
bail:
  return FALSE;
}

gboolean
unserialize_edsiostring_internal (SerialSource *source, SerialEdsioString** result)
{
  SerialEdsioString* unser;
  (*result) = NULL;
  unser = serializeio_source_alloc (source, sizeof (SerialEdsioString));
  if (! unser) goto bail;
  if (! unserialize_edsiostring_internal_noalloc (source, unser)) goto bail;
  (*result) = unser;
  return TRUE;
bail:
  return FALSE;
}

gboolean
unserialize_edsiostring (SerialSource *source, SerialEdsioString** result)
{
  if ( (* source->source_type) (source, TRUE) != ST_EdsioString) goto bail;
  if (! unserialize_edsiostring_internal (source, result)) goto bail;
  if (! serializeio_source_object_received (source)) goto bail;
  return TRUE;
bail:
  return FALSE;
}

/* GenericTime Count
 */

guint
serializeio_count_generictime (guint32 seconds, guint32 nanos) {
  guint size = sizeof (SerialGenericTime);
  ALIGN_8 (size);
  ALIGN_8 (size);
  ALIGN_8 (size);
  return size;
}

guint
serializeio_count_generictime_obj (SerialGenericTime const* obj) {
  return serializeio_count_generictime (obj->seconds, obj->nanos);
}

/* GenericTime Print
 */

void
serializeio_print_generictime_obj (SerialGenericTime* obj, guint indent_spaces) {
  print_spaces (indent_spaces);
  g_print ("[ST_GenericTime]\n");
  print_spaces (indent_spaces);
  g_print ("seconds = ");
  g_print ("%d\n", obj->seconds);
  print_spaces (indent_spaces);
  g_print ("nanos = ");
  g_print ("%d\n", obj->nanos);
}

/* GenericTime Serialize
 */

gboolean
serialize_generictime_internal (SerialSink *sink, guint32 seconds, guint32 nanos)
{
  if (! (* sink->next_uint) (sink, seconds)) goto bail;
  if (! (* sink->next_uint) (sink, nanos)) goto bail;
  return TRUE;
bail:
  return FALSE;
}

gboolean
serialize_generictime_obj_internal (SerialSink *sink, SerialGenericTime* obj)
{
  return serialize_generictime_internal (sink, obj->seconds, obj->nanos);
}

gboolean
serialize_generictime (SerialSink *sink, guint32 seconds, guint32 nanos)
{
  if (! (* sink->sink_type) (sink, ST_GenericTime, serializeio_count_generictime (seconds, nanos), TRUE)) goto bail;
  if (! serialize_generictime_internal (sink, seconds, nanos)) goto bail;
  if (sink->sink_quantum && ! sink->sink_quantum (sink)) goto bail;
  return TRUE;
bail:
  return FALSE;
}

gboolean
serialize_generictime_obj (SerialSink *sink, const SerialGenericTime* obj) {

  return serialize_generictime (sink, obj->seconds, obj->nanos);
}

/* GenericTime Unserialize
 */

gboolean
unserialize_generictime_internal_noalloc (SerialSource *source, SerialGenericTime* result)
{
  if (! (* source->next_uint) (source, &result->seconds)) goto bail;
  if (! (* source->next_uint) (source, &result->nanos)) goto bail;
  return TRUE;
bail:
  return FALSE;
}

gboolean
unserialize_generictime_internal (SerialSource *source, SerialGenericTime** result)
{
  SerialGenericTime* unser;
  (*result) = NULL;
  unser = serializeio_source_alloc (source, sizeof (SerialGenericTime));
  if (! unser) goto bail;
  if (! unserialize_generictime_internal_noalloc (source, unser)) goto bail;
  (*result) = unser;
  return TRUE;
bail:
  return FALSE;
}

gboolean
unserialize_generictime (SerialSource *source, SerialGenericTime** result)
{
  if ( (* source->source_type) (source, TRUE) != ST_GenericTime) goto bail;
  if (! unserialize_generictime_internal (source, result)) goto bail;
  if (! serializeio_source_object_received (source)) goto bail;
  return TRUE;
bail:
  return FALSE;
}

void
edsio_generate_errno_event_internal (EdsioErrnoEventCode _code, const char* _srcfile, gint _srcline)
{
  EdsioErrnoEvent *_e = g_new0 (EdsioErrnoEvent, 1);
  _e->code = _code.code;
  _e->srcline = _srcline;
  _e->srcfile = _srcfile;
  _e->ev_errno = errno;
  eventdelivery_event_deliver ((GenericEvent*) _e);
}

const char*
Edsio_Errno_event_field_to_string (GenericEvent* ev, gint field)
{
  EdsioErrnoEvent* it = (EdsioErrnoEvent*) ev;
  switch (field)
    {
    case 0: return g_strdup (g_strerror (it->ev_errno));
    default: abort ();
    }
}

const EdsioErrnoEventCode EC_EdsioGetTimeOfDayFailure = { EC_EdsioGetTimeOfDayFailureValue };

const EdsioErrnoEventCode EC_EdsioTimeFailure = { EC_EdsioTimeFailureValue };

void
edsio_generate_intint_event_internal (EdsioIntIntEventCode _code, const char* _srcfile, gint _srcline, int library, int number)
{
  EdsioIntIntEvent *_e = g_new0 (EdsioIntIntEvent, 1);
  _e->code = _code.code;
  _e->srcline = _srcline;
  _e->srcfile = _srcfile;
  _e->library = library;
  _e->number = number;
  eventdelivery_event_deliver ((GenericEvent*) _e);
}

const char*
Edsio_IntInt_event_field_to_string (GenericEvent* ev, gint field)
{
  EdsioIntIntEvent* it = (EdsioIntIntEvent*) ev;
  switch (field)
    {
    case 0: return eventdelivery_int_to_string (it->library);
    case 1: return eventdelivery_int_to_string (it->number);
    default: abort ();
    }
}

const EdsioIntIntEventCode EC_EdsioUnregisteredType = { EC_EdsioUnregisteredTypeValue };

const EdsioIntIntEventCode EC_EdsioUnexpectedLibraryType = { EC_EdsioUnexpectedLibraryTypeValue };

void
edsio_generate_void_event_internal (EdsioVoidEventCode _code, const char* _srcfile, gint _srcline)
{
  EdsioVoidEvent *_e = g_new0 (EdsioVoidEvent, 1);
  _e->code = _code.code;
  _e->srcline = _srcline;
  _e->srcfile = _srcfile;
  eventdelivery_event_deliver ((GenericEvent*) _e);
}

const EdsioVoidEventCode EC_EdsioUnexpectedType = { EC_EdsioUnexpectedTypeValue };

const EdsioVoidEventCode EC_EdsioOutputBufferShort = { EC_EdsioOutputBufferShortValue };

const EdsioVoidEventCode EC_EdsioInvalidBase64Encoding = { EC_EdsioInvalidBase64EncodingValue };

const EdsioVoidEventCode EC_EdsioMissingChecksum = { EC_EdsioMissingChecksumValue };

const EdsioVoidEventCode EC_EdsioInvalidChecksum = { EC_EdsioInvalidChecksumValue };

void
edsio_generate_source_event_internal (EdsioSourceEventCode _code, const char* _srcfile, gint _srcline, SerialSource* source)
{
  EdsioSourceEvent *_e = g_new0 (EdsioSourceEvent, 1);
  _e->code = _code.code;
  _e->srcline = _srcline;
  _e->srcfile = _srcfile;
  _e->source = source;
  eventdelivery_event_deliver ((GenericEvent*) _e);
}

const char*
Edsio_Source_event_field_to_string (GenericEvent* ev, gint field)
{
  EdsioSourceEvent* it = (EdsioSourceEvent*) ev;
  switch (field)
    {
    case 0: return eventdelivery_source_to_string (it->source);
    default: abort ();
    }
}

const EdsioSourceEventCode EC_EdsioSourceEof = { EC_EdsioSourceEofValue };

const EdsioSourceEventCode EC_EdsioIncorrectAllocation = { EC_EdsioIncorrectAllocationValue };

void
edsio_generate_stringstring_event_internal (EdsioStringStringEventCode _code, const char* _srcfile, gint _srcline, const char* msg, const char* arg)
{
  EdsioStringStringEvent *_e = g_new0 (EdsioStringStringEvent, 1);
  _e->code = _code.code;
  _e->srcline = _srcline;
  _e->srcfile = _srcfile;
  _e->msg = msg;
  _e->arg = arg;
  eventdelivery_event_deliver ((GenericEvent*) _e);
}

const char*
Edsio_StringString_event_field_to_string (GenericEvent* ev, gint field)
{
  EdsioStringStringEvent* it = (EdsioStringStringEvent*) ev;
  switch (field)
    {
    case 0: return eventdelivery_string_to_string (it->msg);
    case 1: return eventdelivery_string_to_string (it->arg);
    default: abort ();
    }
}

const EdsioStringStringEventCode EC_EdsioInvalidIntegerString = { EC_EdsioInvalidIntegerStringValue };

const EdsioStringStringEventCode EC_EdsioIntegerOutOfRange = { EC_EdsioIntegerOutOfRangeValue };

const EdsioStringStringEventCode EC_EdsioInvalidIntegerSign = { EC_EdsioInvalidIntegerSignValue };

void
edsio_generate_string_event_internal (EdsioStringEventCode _code, const char* _srcfile, gint _srcline, const char* name)
{
  EdsioStringEvent *_e = g_new0 (EdsioStringEvent, 1);
  _e->code = _code.code;
  _e->srcline = _srcline;
  _e->srcfile = _srcfile;
  _e->name = name;
  eventdelivery_event_deliver ((GenericEvent*) _e);
}

const char*
Edsio_String_event_field_to_string (GenericEvent* ev, gint field)
{
  EdsioStringEvent* it = (EdsioStringEvent*) ev;
  switch (field)
    {
    case 0: return eventdelivery_string_to_string (it->name);
    default: abort ();
    }
}

const EdsioStringEventCode EC_EdsioDuplicatePropertyTypeRegistered = { EC_EdsioDuplicatePropertyTypeRegisteredValue };

const EdsioStringEventCode EC_EdsioDuplicateHostTypeRegistered = { EC_EdsioDuplicateHostTypeRegisteredValue };

const EdsioStringEventCode EC_EdsioDuplicatePropertyNameRegistered = { EC_EdsioDuplicatePropertyNameRegisteredValue };

void
edsio_generate_int_event_internal (EdsioIntEventCode _code, const char* _srcfile, gint _srcline, int num)
{
  EdsioIntEvent *_e = g_new0 (EdsioIntEvent, 1);
  _e->code = _code.code;
  _e->srcline = _srcline;
  _e->srcfile = _srcfile;
  _e->num = num;
  eventdelivery_event_deliver ((GenericEvent*) _e);
}

const char*
Edsio_Int_event_field_to_string (GenericEvent* ev, gint field)
{
  EdsioIntEvent* it = (EdsioIntEvent*) ev;
  switch (field)
    {
    case 0: return eventdelivery_int_to_string (it->num);
    default: abort ();
    }
}

const EdsioIntEventCode EC_EdsioNoSuchProperty = { EC_EdsioNoSuchPropertyValue };

const EdsioStringEventCode EC_EdsioNoSuchPropertyType = { EC_EdsioNoSuchPropertyTypeValue };

const EdsioStringEventCode EC_EdsioNoSuchHostType = { EC_EdsioNoSuchHostTypeValue };

void
edsio_generate_stringstringstring_event_internal (EdsioStringStringStringEventCode _code, const char* _srcfile, gint _srcline, const char* name, const char* recv, const char* expect)
{
  EdsioStringStringStringEvent *_e = g_new0 (EdsioStringStringStringEvent, 1);
  _e->code = _code.code;
  _e->srcline = _srcline;
  _e->srcfile = _srcfile;
  _e->name = name;
  _e->recv = recv;
  _e->expect = expect;
  eventdelivery_event_deliver ((GenericEvent*) _e);
}

const char*
Edsio_StringStringString_event_field_to_string (GenericEvent* ev, gint field)
{
  EdsioStringStringStringEvent* it = (EdsioStringStringStringEvent*) ev;
  switch (field)
    {
    case 0: return eventdelivery_string_to_string (it->name);
    case 1: return eventdelivery_string_to_string (it->recv);
    case 2: return eventdelivery_string_to_string (it->expect);
    default: abort ();
    }
}

const EdsioStringStringStringEventCode EC_EdsioWrongHostType = { EC_EdsioWrongHostTypeValue };

const EdsioStringStringStringEventCode EC_EdsioWrongDataType = { EC_EdsioWrongDataTypeValue };

const EdsioStringEventCode EC_EdsioPropertyNotSet = { EC_EdsioPropertyNotSetValue };

const EdsioStringStringEventCode EC_EdsioPersistenceUnavailable = { EC_EdsioPersistenceUnavailableValue };

const EdsioVoidEventCode EC_EdsioInvalidStreamChecksum = { EC_EdsioInvalidStreamChecksumValue };

const EdsioStringStringEventCode EC_EdsioInvalidHexDigit = { EC_EdsioInvalidHexDigitValue };

const EdsioStringEventCode EC_EdsioMD5StringShort = { EC_EdsioMD5StringShortValue };

const EdsioStringEventCode EC_EdsioMD5StringLong = { EC_EdsioMD5StringLongValue };

const EdsioIntEventCode EC_EdsioUnregisteredLibrary = { EC_EdsioUnregisteredLibraryValue };

const EdsioStringStringEventCode EC_EdsioGModuleError = { EC_EdsioGModuleErrorValue };

