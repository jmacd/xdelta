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

#include "xdelta.h"

#include <errno.h>

/* Declarations. */

static const char* Xd_String_event_field_to_string (GenericEvent* ev, gint field);
static const char* Xd_HandleIntInt_event_field_to_string (GenericEvent* ev, gint field);
static const char* Xd_HandleStringString_event_field_to_string (GenericEvent* ev, gint field);
static const char* Xd_Int_event_field_to_string (GenericEvent* ev, gint field);
static void print_spaces (guint n) { int i; for (i = 0; i < n; i += 1) g_print (" "); }


/* initialize this library. */

gboolean
xd_edsio_init (void)
{
  static gboolean once = FALSE;
  static gboolean result = FALSE;
  if (once) return result;
  once = TRUE;
  eventdelivery_initialize_event_def (EC_XdBackwardCompatibilityModeValue, EL_Information, EF_None, "BackwardCompatibilityMode", "Reading a version ${0} delta control", & Xd_String_event_field_to_string);
  eventdelivery_initialize_event_def (EC_XdStreamLengthFailedValue, EL_Error, EF_None, "StreamLengthFailed", "${0}: Length validation failed, expected: ${1}, received: ${2}", & Xd_HandleIntInt_event_field_to_string);
  eventdelivery_initialize_event_def (EC_XdStreamChecksumFailedValue, EL_Error, EF_None, "StreamChecksumFailed", "${0}: Checksum validation failed, expected: ${1}, received: ${2}", & Xd_HandleStringString_event_field_to_string);
  eventdelivery_initialize_event_def (EC_XdIncompatibleDeltaValue, EL_Error, EF_None, "IncompatibleDelta", "The delta was not produced according by the `xdelta delta' command", NULL);
  eventdelivery_initialize_event_def (EC_XdInvalidRsyncCacheValue, EL_Error, EF_None, "InvalidRsyncCache", "The rsync checksum cache is corrupt", NULL);
  eventdelivery_initialize_event_def (EC_XdInvalidControlValue, EL_Error, EF_None, "InvalidControl", "Delta control is corrupt", NULL);
  eventdelivery_initialize_event_def (EC_XdOutOfRangeSourceIndexValue, EL_Error, EF_None, "OutOfRangeSourceIndex", "Instruction references out-of-range source index: ${0}", & Xd_Int_event_field_to_string);
  eventdelivery_initialize_event_def (EC_XdTooManySourcesValue, EL_Error, EF_None, "TooManySources", "Too many input sources", NULL);
  eventdelivery_initialize_event_def (EC_XdTooFewSourcesValue, EL_Error, EF_None, "TooFewSources", "Too few input sources", NULL);
  serializeio_initialize_type ("ST_Version0Instruction", ST_Version0Instruction, &unserialize_version0instruction_internal, &serialize_version0instruction_obj_internal, &serializeio_count_version0instruction_obj, &serializeio_print_version0instruction_obj);
  serializeio_initialize_type ("ST_Version0Control", ST_Version0Control, &unserialize_version0control_internal, &serialize_version0control_obj_internal, &serializeio_count_version0control_obj, &serializeio_print_version0control_obj);
  serializeio_initialize_type ("ST_Version0SourceInfo", ST_Version0SourceInfo, &unserialize_version0sourceinfo_internal, &serialize_version0sourceinfo_obj_internal, &serializeio_count_version0sourceinfo_obj, &serializeio_print_version0sourceinfo_obj);
  serializeio_initialize_type ("ST_RsyncIndex", ST_RsyncIndex, &unserialize_rsyncindex_internal, &serialize_rsyncindex_obj_internal, &serializeio_count_rsyncindex_obj, &serializeio_print_rsyncindex_obj);
  serializeio_initialize_type ("ST_RsyncIndexElt", ST_RsyncIndexElt, &unserialize_rsyncindexelt_internal, &serialize_rsyncindexelt_obj_internal, &serializeio_count_rsyncindexelt_obj, &serializeio_print_rsyncindexelt_obj);
  serializeio_initialize_type ("ST_XdeltaInstruction", ST_XdeltaInstruction, &unserialize_xdeltainstruction_internal, &serialize_xdeltainstruction_obj_internal, &serializeio_count_xdeltainstruction_obj, &serializeio_print_xdeltainstruction_obj);
  serializeio_initialize_type ("ST_XdeltaControl", ST_XdeltaControl, &unserialize_xdeltacontrol_internal, &serialize_xdeltacontrol_obj_internal, &serializeio_count_xdeltacontrol_obj, &serializeio_print_xdeltacontrol_obj);
  serializeio_initialize_type ("ST_XdeltaSourceInfo", ST_XdeltaSourceInfo, &unserialize_xdeltasourceinfo_internal, &serialize_xdeltasourceinfo_obj_internal, &serializeio_count_xdeltasourceinfo_obj, &serializeio_print_xdeltasourceinfo_obj);
  serializeio_initialize_type ("ST_XdeltaIndex", ST_XdeltaIndex, &unserialize_xdeltaindex_internal, &serialize_xdeltaindex_obj_internal, &serializeio_count_xdeltaindex_obj, &serializeio_print_xdeltaindex_obj);
  serializeio_initialize_type ("ST_XdeltaChecksum", ST_XdeltaChecksum, &unserialize_xdeltachecksum_internal, &serialize_xdeltachecksum_obj_internal, &serializeio_count_xdeltachecksum_obj, &serializeio_print_xdeltachecksum_obj);
  edsio_library_register (3, "xd");
  result = TRUE;
  return TRUE;
}

/* XdeltaChecksum Count
 */

guint
serializeio_count_xdeltachecksum (guint16 high, guint16 low) {
  guint size = sizeof (SerialXdeltaChecksum);
  ALIGN_8 (size);
  ALIGN_8 (size);
  ALIGN_8 (size);
  return size;
}

guint
serializeio_count_xdeltachecksum_obj (SerialXdeltaChecksum const* obj) {
  return serializeio_count_xdeltachecksum (obj->high, obj->low);
}

/* XdeltaChecksum Print
 */

void
serializeio_print_xdeltachecksum_obj (SerialXdeltaChecksum* obj, guint indent_spaces) {
  print_spaces (indent_spaces);
  g_print ("[ST_XdeltaChecksum]\n");
  print_spaces (indent_spaces);
  g_print ("high = ");
  g_print ("%d\n", obj->high);
  print_spaces (indent_spaces);
  g_print ("low = ");
  g_print ("%d\n", obj->low);
}

/* XdeltaChecksum Serialize
 */

gboolean
serialize_xdeltachecksum_internal (SerialSink *sink, guint16 high, guint16 low)
{
  if (! (* sink->next_uint16) (sink, high)) goto bail;
  if (! (* sink->next_uint16) (sink, low)) goto bail;
  return TRUE;
bail:
  return FALSE;
}

gboolean
serialize_xdeltachecksum_obj_internal (SerialSink *sink, SerialXdeltaChecksum* obj)
{
  return serialize_xdeltachecksum_internal (sink, obj->high, obj->low);
}

gboolean
serialize_xdeltachecksum (SerialSink *sink, guint16 high, guint16 low)
{
  if (! (* sink->sink_type) (sink, ST_XdeltaChecksum, serializeio_count_xdeltachecksum (high, low), TRUE)) goto bail;
  if (! serialize_xdeltachecksum_internal (sink, high, low)) goto bail;
  if (sink->sink_quantum && ! sink->sink_quantum (sink)) goto bail;
  return TRUE;
bail:
  return FALSE;
}

gboolean
serialize_xdeltachecksum_obj (SerialSink *sink, const SerialXdeltaChecksum* obj) {

  return serialize_xdeltachecksum (sink, obj->high, obj->low);
}

/* XdeltaChecksum Unserialize
 */

gboolean
unserialize_xdeltachecksum_internal_noalloc (SerialSource *source, SerialXdeltaChecksum* result)
{
  if (! (* source->next_uint16) (source, &result->high)) goto bail;
  if (! (* source->next_uint16) (source, &result->low)) goto bail;
  return TRUE;
bail:
  return FALSE;
}

gboolean
unserialize_xdeltachecksum_internal (SerialSource *source, SerialXdeltaChecksum** result)
{
  SerialXdeltaChecksum* unser;
  (*result) = NULL;
  unser = serializeio_source_alloc (source, sizeof (SerialXdeltaChecksum));
  if (! unser) goto bail;
  if (! unserialize_xdeltachecksum_internal_noalloc (source, unser)) goto bail;
  (*result) = unser;
  return TRUE;
bail:
  return FALSE;
}

gboolean
unserialize_xdeltachecksum (SerialSource *source, SerialXdeltaChecksum** result)
{
  if ( (* source->source_type) (source, TRUE) != ST_XdeltaChecksum) goto bail;
  if (! unserialize_xdeltachecksum_internal (source, result)) goto bail;
  if (! serializeio_source_object_received (source)) goto bail;
  return TRUE;
bail:
  return FALSE;
}

/* XdeltaIndex Count
 */

guint
serializeio_count_xdeltaindex (guint32 file_len, const guint8 file_md5[16], guint32 index_len, SerialXdeltaChecksum const* index) {
  guint size = sizeof (SerialXdeltaIndex);
  ALIGN_8 (size);
  ALIGN_8 (size);
  ALIGN_8 (size);
  {
    gint i;
    for (i = 0; i < index_len; i += 1)
      {
        size += serializeio_count_xdeltachecksum_obj (& (index[i]));
      }
  }
  ALIGN_8 (size);
  return size;
}

guint
serializeio_count_xdeltaindex_obj (SerialXdeltaIndex const* obj) {
  return serializeio_count_xdeltaindex (obj->file_len, obj->file_md5, obj->index_len, obj->index);
}

/* XdeltaIndex Print
 */

void
serializeio_print_xdeltaindex_obj (SerialXdeltaIndex* obj, guint indent_spaces) {
  print_spaces (indent_spaces);
  g_print ("[ST_XdeltaIndex]\n");
  print_spaces (indent_spaces);
  g_print ("file_len = ");
  g_print ("%d\n", obj->file_len);
  print_spaces (indent_spaces);
  g_print ("file_md5 = ");
  serializeio_print_bytes (obj->file_md5, 16);
  print_spaces (indent_spaces);
  g_print ("index = ");
  g_print ("{\n");
  {
    gint i;
    for (i = 0; i < obj->index_len; i += 1)
      {
        print_spaces (indent_spaces);
        g_print ("%d: ", i);
        print_spaces (indent_spaces);
      serializeio_print_xdeltachecksum_obj (& (obj->index[i]), indent_spaces + 2);
      print_spaces (indent_spaces);
;
      }
  }
  g_print ("}\n");
}

/* XdeltaIndex Serialize
 */

gboolean
serialize_xdeltaindex_internal (SerialSink *sink, guint32 file_len, const guint8 file_md5[16], guint32 index_len, SerialXdeltaChecksum const* index)
{
  if (! (* sink->next_uint) (sink, file_len)) goto bail;
  if (! (* sink->next_bytes_known) (sink, file_md5, 16)) goto bail;
  {
    gint i;
    if (! (* sink->next_uint) (sink, index_len)) goto bail;
    for (i = 0; i < index_len; i += 1)
      {
        if (! serialize_xdeltachecksum_internal (sink, (index[i]).high, (index[i]).low)) goto bail;
      }
  }
  return TRUE;
bail:
  return FALSE;
}

gboolean
serialize_xdeltaindex_obj_internal (SerialSink *sink, SerialXdeltaIndex* obj)
{
  return serialize_xdeltaindex_internal (sink, obj->file_len, obj->file_md5, obj->index_len, obj->index);
}

gboolean
serialize_xdeltaindex (SerialSink *sink, guint32 file_len, const guint8 file_md5[16], guint32 index_len, SerialXdeltaChecksum const* index)
{
  if (! (* sink->sink_type) (sink, ST_XdeltaIndex, serializeio_count_xdeltaindex (file_len, file_md5, index_len, index), TRUE)) goto bail;
  if (! serialize_xdeltaindex_internal (sink, file_len, file_md5, index_len, index)) goto bail;
  if (sink->sink_quantum && ! sink->sink_quantum (sink)) goto bail;
  return TRUE;
bail:
  return FALSE;
}

gboolean
serialize_xdeltaindex_obj (SerialSink *sink, const SerialXdeltaIndex* obj) {

  return serialize_xdeltaindex (sink, obj->file_len, obj->file_md5, obj->index_len, obj->index);
}

/* XdeltaIndex Unserialize
 */

gboolean
unserialize_xdeltaindex_internal_noalloc (SerialSource *source, SerialXdeltaIndex* result)
{
  if (! (* source->next_uint) (source, &result->file_len)) goto bail;
  if (! (* source->next_bytes_known) (source, result->file_md5, 16)) goto bail;
  {
    gint i;
    if (! (* source->next_uint) (source, &result->index_len)) goto bail;
    if (! (result->index = serializeio_source_alloc (source, sizeof (SerialXdeltaChecksum) * result->index_len))) goto bail;
    for (i = 0; i < result->index_len; i += 1)
      {
        if (! unserialize_xdeltachecksum_internal_noalloc (source, &(result->index[i]))) goto bail;
      }
  }
  return TRUE;
bail:
  return FALSE;
}

gboolean
unserialize_xdeltaindex_internal (SerialSource *source, SerialXdeltaIndex** result)
{
  SerialXdeltaIndex* unser;
  (*result) = NULL;
  unser = serializeio_source_alloc (source, sizeof (SerialXdeltaIndex));
  if (! unser) goto bail;
  if (! unserialize_xdeltaindex_internal_noalloc (source, unser)) goto bail;
  (*result) = unser;
  return TRUE;
bail:
  return FALSE;
}

gboolean
unserialize_xdeltaindex (SerialSource *source, SerialXdeltaIndex** result)
{
  if ( (* source->source_type) (source, TRUE) != ST_XdeltaIndex) goto bail;
  if (! unserialize_xdeltaindex_internal (source, result)) goto bail;
  if (! serializeio_source_object_received (source)) goto bail;
  return TRUE;
bail:
  return FALSE;
}

/* XdeltaSourceInfo Count
 */

guint
serializeio_count_xdeltasourceinfo (const gchar* name, const guint8 md5[16], guint32 len, gboolean isdata, gboolean sequential) {
  guint size = sizeof (SerialXdeltaSourceInfo);
  ALIGN_8 (size);
  size += strlen (name) + 1;
  ALIGN_8 (size);
  ALIGN_8 (size);
  ALIGN_8 (size);
  ALIGN_8 (size);
  ALIGN_8 (size);
  return size;
}

guint
serializeio_count_xdeltasourceinfo_obj (SerialXdeltaSourceInfo const* obj) {
  return serializeio_count_xdeltasourceinfo (obj->name, obj->md5, obj->len, obj->isdata, obj->sequential);
}

/* XdeltaSourceInfo Print
 */

void
serializeio_print_xdeltasourceinfo_obj (SerialXdeltaSourceInfo* obj, guint indent_spaces) {
  print_spaces (indent_spaces);
  g_print ("[ST_XdeltaSourceInfo]\n");
  print_spaces (indent_spaces);
  g_print ("name = ");
  g_print ("%s\n", obj->name);
  print_spaces (indent_spaces);
  g_print ("md5 = ");
  serializeio_print_bytes (obj->md5, 16);
  print_spaces (indent_spaces);
  g_print ("len = ");
  g_print ("%d\n", obj->len);
  print_spaces (indent_spaces);
  g_print ("isdata = ");
  g_print ("%s\n", obj->isdata ? "true" : "false");
  print_spaces (indent_spaces);
  g_print ("sequential = ");
  g_print ("%s\n", obj->sequential ? "true" : "false");
}

/* XdeltaSourceInfo Serialize
 */

gboolean
serialize_xdeltasourceinfo_internal (SerialSink *sink, const gchar* name, const guint8 md5[16], guint32 len, gboolean isdata, gboolean sequential)
{
  if (! (* sink->next_string) (sink, name)) goto bail;
  if (! (* sink->next_bytes_known) (sink, md5, 16)) goto bail;
  if (! (* sink->next_uint) (sink, len)) goto bail;
  if (! (* sink->next_bool) (sink, isdata)) goto bail;
  if (! (* sink->next_bool) (sink, sequential)) goto bail;
  return TRUE;
bail:
  return FALSE;
}

gboolean
serialize_xdeltasourceinfo_obj_internal (SerialSink *sink, SerialXdeltaSourceInfo* obj)
{
  return serialize_xdeltasourceinfo_internal (sink, obj->name, obj->md5, obj->len, obj->isdata, obj->sequential);
}

gboolean
serialize_xdeltasourceinfo (SerialSink *sink, const gchar* name, const guint8 md5[16], guint32 len, gboolean isdata, gboolean sequential)
{
  if (! (* sink->sink_type) (sink, ST_XdeltaSourceInfo, serializeio_count_xdeltasourceinfo (name, md5, len, isdata, sequential), TRUE)) goto bail;
  if (! serialize_xdeltasourceinfo_internal (sink, name, md5, len, isdata, sequential)) goto bail;
  if (sink->sink_quantum && ! sink->sink_quantum (sink)) goto bail;
  return TRUE;
bail:
  return FALSE;
}

gboolean
serialize_xdeltasourceinfo_obj (SerialSink *sink, const SerialXdeltaSourceInfo* obj) {

  return serialize_xdeltasourceinfo (sink, obj->name, obj->md5, obj->len, obj->isdata, obj->sequential);
}

/* XdeltaSourceInfo Unserialize
 */

gboolean
unserialize_xdeltasourceinfo_internal_noalloc (SerialSource *source, SerialXdeltaSourceInfo* result)
{
  if (! (* source->next_string) (source, &result->name)) goto bail;
  if (! (* source->next_bytes_known) (source, result->md5, 16)) goto bail;
  if (! (* source->next_uint) (source, &result->len)) goto bail;
  if (! (* source->next_bool) (source, &result->isdata)) goto bail;
  if (! (* source->next_bool) (source, &result->sequential)) goto bail;
  return TRUE;
bail:
  return FALSE;
}

gboolean
unserialize_xdeltasourceinfo_internal (SerialSource *source, SerialXdeltaSourceInfo** result)
{
  SerialXdeltaSourceInfo* unser;
  (*result) = NULL;
  unser = serializeio_source_alloc (source, sizeof (SerialXdeltaSourceInfo));
  if (! unser) goto bail;
  if (! unserialize_xdeltasourceinfo_internal_noalloc (source, unser)) goto bail;
  (*result) = unser;
  return TRUE;
bail:
  return FALSE;
}

gboolean
unserialize_xdeltasourceinfo (SerialSource *source, SerialXdeltaSourceInfo** result)
{
  if ( (* source->source_type) (source, TRUE) != ST_XdeltaSourceInfo) goto bail;
  if (! unserialize_xdeltasourceinfo_internal (source, result)) goto bail;
  if (! serializeio_source_object_received (source)) goto bail;
  return TRUE;
bail:
  return FALSE;
}

/* XdeltaControl Count
 */

guint
serializeio_count_xdeltacontrol (const guint8 to_md5[16], guint32 to_len, gboolean has_data, guint32 source_info_len, SerialXdeltaSourceInfo* const* source_info, guint32 inst_len, SerialXdeltaInstruction const* inst) {
  guint size = sizeof (SerialXdeltaControl);
  ALIGN_8 (size);
  ALIGN_8 (size);
  ALIGN_8 (size);
  ALIGN_8 (size);
  {
    gint i;
    for (i = 0; i < source_info_len; i += 1)
      {
        size += serializeio_count_xdeltasourceinfo_obj ((source_info[i])) + sizeof (void*);
      }
  }
  ALIGN_8 (size);
  {
    gint i;
    for (i = 0; i < inst_len; i += 1)
      {
        size += serializeio_count_xdeltainstruction_obj (& (inst[i]));
      }
  }
  ALIGN_8 (size);
  return size;
}

guint
serializeio_count_xdeltacontrol_obj (SerialXdeltaControl const* obj) {
  return serializeio_count_xdeltacontrol (obj->to_md5, obj->to_len, obj->has_data, obj->source_info_len, obj->source_info, obj->inst_len, obj->inst);
}

/* XdeltaControl Print
 */

void
serializeio_print_xdeltacontrol_obj (SerialXdeltaControl* obj, guint indent_spaces) {
  print_spaces (indent_spaces);
  g_print ("[ST_XdeltaControl]\n");
  print_spaces (indent_spaces);
  g_print ("to_md5 = ");
  serializeio_print_bytes (obj->to_md5, 16);
  print_spaces (indent_spaces);
  g_print ("to_len = ");
  g_print ("%d\n", obj->to_len);
  print_spaces (indent_spaces);
  g_print ("has_data = ");
  g_print ("%s\n", obj->has_data ? "true" : "false");
  print_spaces (indent_spaces);
  g_print ("source_info = ");
  g_print ("{\n");
  {
    gint i;
    for (i = 0; i < obj->source_info_len; i += 1)
      {
        print_spaces (indent_spaces);
        g_print ("%d: ", i);
        print_spaces (indent_spaces);
      serializeio_print_xdeltasourceinfo_obj ((obj->source_info[i]), indent_spaces + 2);
      print_spaces (indent_spaces);
;
      }
  }
  g_print ("}\n");
  print_spaces (indent_spaces);
  g_print ("inst = ");
  g_print ("{\n");
  {
    gint i;
    for (i = 0; i < obj->inst_len; i += 1)
      {
        print_spaces (indent_spaces);
        g_print ("%d: ", i);
        print_spaces (indent_spaces);
      serializeio_print_xdeltainstruction_obj (& (obj->inst[i]), indent_spaces + 2);
      print_spaces (indent_spaces);
;
      }
  }
  g_print ("}\n");
}

/* XdeltaControl Serialize
 */

gboolean
serialize_xdeltacontrol_internal (SerialSink *sink, const guint8 to_md5[16], guint32 to_len, gboolean has_data, guint32 source_info_len, SerialXdeltaSourceInfo* const* source_info, guint32 inst_len, SerialXdeltaInstruction const* inst)
{
  if (! (* sink->next_bytes_known) (sink, to_md5, 16)) goto bail;
  if (! (* sink->next_uint) (sink, to_len)) goto bail;
  if (! (* sink->next_bool) (sink, has_data)) goto bail;
  {
    gint i;
    if (! (* sink->next_uint) (sink, source_info_len)) goto bail;
    for (i = 0; i < source_info_len; i += 1)
      {
        if (! serialize_xdeltasourceinfo_internal (sink, (source_info[i])->name, (source_info[i])->md5, (source_info[i])->len, (source_info[i])->isdata, (source_info[i])->sequential)) goto bail;
      }
  }
  {
    gint i;
    if (! (* sink->next_uint) (sink, inst_len)) goto bail;
    for (i = 0; i < inst_len; i += 1)
      {
        if (! serialize_xdeltainstruction_internal (sink, (inst[i]).index, (inst[i]).offset, (inst[i]).length)) goto bail;
      }
  }
  return TRUE;
bail:
  return FALSE;
}

gboolean
serialize_xdeltacontrol_obj_internal (SerialSink *sink, SerialXdeltaControl* obj)
{
  return serialize_xdeltacontrol_internal (sink, obj->to_md5, obj->to_len, obj->has_data, obj->source_info_len, obj->source_info, obj->inst_len, obj->inst);
}

gboolean
serialize_xdeltacontrol (SerialSink *sink, const guint8 to_md5[16], guint32 to_len, gboolean has_data, guint32 source_info_len, SerialXdeltaSourceInfo* const* source_info, guint32 inst_len, SerialXdeltaInstruction const* inst)
{
  if (! (* sink->sink_type) (sink, ST_XdeltaControl, serializeio_count_xdeltacontrol (to_md5, to_len, has_data, source_info_len, source_info, inst_len, inst), TRUE)) goto bail;
  if (! serialize_xdeltacontrol_internal (sink, to_md5, to_len, has_data, source_info_len, source_info, inst_len, inst)) goto bail;
  if (sink->sink_quantum && ! sink->sink_quantum (sink)) goto bail;
  return TRUE;
bail:
  return FALSE;
}

gboolean
serialize_xdeltacontrol_obj (SerialSink *sink, const SerialXdeltaControl* obj) {

  return serialize_xdeltacontrol (sink, obj->to_md5, obj->to_len, obj->has_data, obj->source_info_len, obj->source_info, obj->inst_len, obj->inst);
}

/* XdeltaControl Unserialize
 */

gboolean
unserialize_xdeltacontrol_internal_noalloc (SerialSource *source, SerialXdeltaControl* result)
{
  if (! (* source->next_bytes_known) (source, result->to_md5, 16)) goto bail;
  if (! (* source->next_uint) (source, &result->to_len)) goto bail;
  if (! (* source->next_bool) (source, &result->has_data)) goto bail;
  {
    gint i;
    if (! (* source->next_uint) (source, &result->source_info_len)) goto bail;
    if (! (result->source_info = serializeio_source_alloc (source, sizeof (SerialXdeltaSourceInfo*) * result->source_info_len))) goto bail;
    for (i = 0; i < result->source_info_len; i += 1)
      {
        if (! unserialize_xdeltasourceinfo_internal (source, &(result->source_info[i]))) goto bail;
      }
  }
  {
    gint i;
    if (! (* source->next_uint) (source, &result->inst_len)) goto bail;
    if (! (result->inst = serializeio_source_alloc (source, sizeof (SerialXdeltaInstruction) * result->inst_len))) goto bail;
    for (i = 0; i < result->inst_len; i += 1)
      {
        if (! unserialize_xdeltainstruction_internal_noalloc (source, &(result->inst[i]))) goto bail;
      }
  }
  return TRUE;
bail:
  return FALSE;
}

gboolean
unserialize_xdeltacontrol_internal (SerialSource *source, SerialXdeltaControl** result)
{
  SerialXdeltaControl* unser;
  (*result) = NULL;
  unser = serializeio_source_alloc (source, sizeof (SerialXdeltaControl));
  if (! unser) goto bail;
  if (! unserialize_xdeltacontrol_internal_noalloc (source, unser)) goto bail;
  (*result) = unser;
  return TRUE;
bail:
  return FALSE;
}

gboolean
unserialize_xdeltacontrol (SerialSource *source, SerialXdeltaControl** result)
{
  if ( (* source->source_type) (source, TRUE) != ST_XdeltaControl) goto bail;
  if (! unserialize_xdeltacontrol_internal (source, result)) goto bail;
  if (! serializeio_source_object_received (source)) goto bail;
  return TRUE;
bail:
  return FALSE;
}

/* XdeltaInstruction Count
 */

guint
serializeio_count_xdeltainstruction (guint32 index, guint32 offset, guint32 length) {
  guint size = sizeof (SerialXdeltaInstruction);
  ALIGN_8 (size);
  ALIGN_8 (size);
  ALIGN_8 (size);
  ALIGN_8 (size);
  return size;
}

guint
serializeio_count_xdeltainstruction_obj (SerialXdeltaInstruction const* obj) {
  return serializeio_count_xdeltainstruction (obj->index, obj->offset, obj->length);
}

/* XdeltaInstruction Print
 */

void
serializeio_print_xdeltainstruction_obj (SerialXdeltaInstruction* obj, guint indent_spaces) {
  print_spaces (indent_spaces);
  g_print ("[ST_XdeltaInstruction]\n");
  print_spaces (indent_spaces);
  g_print ("index = ");
  g_print ("%d\n", obj->index);
  print_spaces (indent_spaces);
  g_print ("offset = ");
  g_print ("%d\n", obj->offset);
  print_spaces (indent_spaces);
  g_print ("length = ");
  g_print ("%d\n", obj->length);
}

/* XdeltaInstruction Serialize
 */

gboolean
serialize_xdeltainstruction_internal (SerialSink *sink, guint32 index, guint32 offset, guint32 length)
{
  if (! (* sink->next_uint) (sink, index)) goto bail;
  if (! (* sink->next_uint) (sink, offset)) goto bail;
  if (! (* sink->next_uint) (sink, length)) goto bail;
  return TRUE;
bail:
  return FALSE;
}

gboolean
serialize_xdeltainstruction_obj_internal (SerialSink *sink, SerialXdeltaInstruction* obj)
{
  return serialize_xdeltainstruction_internal (sink, obj->index, obj->offset, obj->length);
}

gboolean
serialize_xdeltainstruction (SerialSink *sink, guint32 index, guint32 offset, guint32 length)
{
  if (! (* sink->sink_type) (sink, ST_XdeltaInstruction, serializeio_count_xdeltainstruction (index, offset, length), TRUE)) goto bail;
  if (! serialize_xdeltainstruction_internal (sink, index, offset, length)) goto bail;
  if (sink->sink_quantum && ! sink->sink_quantum (sink)) goto bail;
  return TRUE;
bail:
  return FALSE;
}

gboolean
serialize_xdeltainstruction_obj (SerialSink *sink, const SerialXdeltaInstruction* obj) {

  return serialize_xdeltainstruction (sink, obj->index, obj->offset, obj->length);
}

/* XdeltaInstruction Unserialize
 */

gboolean
unserialize_xdeltainstruction_internal_noalloc (SerialSource *source, SerialXdeltaInstruction* result)
{
  if (! (* source->next_uint) (source, &result->index)) goto bail;
  if (! (* source->next_uint) (source, &result->offset)) goto bail;
  if (! (* source->next_uint) (source, &result->length)) goto bail;
  return TRUE;
bail:
  return FALSE;
}

gboolean
unserialize_xdeltainstruction_internal (SerialSource *source, SerialXdeltaInstruction** result)
{
  SerialXdeltaInstruction* unser;
  (*result) = NULL;
  unser = serializeio_source_alloc (source, sizeof (SerialXdeltaInstruction));
  if (! unser) goto bail;
  if (! unserialize_xdeltainstruction_internal_noalloc (source, unser)) goto bail;
  (*result) = unser;
  return TRUE;
bail:
  return FALSE;
}

gboolean
unserialize_xdeltainstruction (SerialSource *source, SerialXdeltaInstruction** result)
{
  if ( (* source->source_type) (source, TRUE) != ST_XdeltaInstruction) goto bail;
  if (! unserialize_xdeltainstruction_internal (source, result)) goto bail;
  if (! serializeio_source_object_received (source)) goto bail;
  return TRUE;
bail:
  return FALSE;
}

/* RsyncIndexElt Count
 */

guint
serializeio_count_rsyncindexelt (const guint8 md5[16], SerialXdeltaChecksum const* cksum) {
  guint size = sizeof (SerialRsyncIndexElt);
  ALIGN_8 (size);
  ALIGN_8 (size);
  size += serializeio_count_xdeltachecksum_obj (cksum) - sizeof (SerialXdeltaChecksum);
  ALIGN_8 (size);
  return size;
}

guint
serializeio_count_rsyncindexelt_obj (SerialRsyncIndexElt const* obj) {
  return serializeio_count_rsyncindexelt (obj->md5, &obj->cksum);
}

/* RsyncIndexElt Print
 */

void
serializeio_print_rsyncindexelt_obj (SerialRsyncIndexElt* obj, guint indent_spaces) {
  print_spaces (indent_spaces);
  g_print ("[ST_RsyncIndexElt]\n");
  print_spaces (indent_spaces);
  g_print ("md5 = ");
  serializeio_print_bytes (obj->md5, 16);
  print_spaces (indent_spaces);
  g_print ("cksum = ");
  g_print ("{\n");
  serializeio_print_xdeltachecksum_obj (& obj->cksum, indent_spaces + 2);
  print_spaces (indent_spaces);
;
  g_print ("}\n");
}

/* RsyncIndexElt Serialize
 */

gboolean
serialize_rsyncindexelt_internal (SerialSink *sink, const guint8 md5[16], SerialXdeltaChecksum const* cksum)
{
  if (! (* sink->next_bytes_known) (sink, md5, 16)) goto bail;
  if (! serialize_xdeltachecksum_internal (sink, cksum->high, cksum->low)) goto bail;
  return TRUE;
bail:
  return FALSE;
}

gboolean
serialize_rsyncindexelt_obj_internal (SerialSink *sink, SerialRsyncIndexElt* obj)
{
  return serialize_rsyncindexelt_internal (sink, obj->md5, &obj->cksum);
}

gboolean
serialize_rsyncindexelt (SerialSink *sink, const guint8 md5[16], SerialXdeltaChecksum const* cksum)
{
  if (! (* sink->sink_type) (sink, ST_RsyncIndexElt, serializeio_count_rsyncindexelt (md5, cksum), TRUE)) goto bail;
  if (! serialize_rsyncindexelt_internal (sink, md5, cksum)) goto bail;
  if (sink->sink_quantum && ! sink->sink_quantum (sink)) goto bail;
  return TRUE;
bail:
  return FALSE;
}

gboolean
serialize_rsyncindexelt_obj (SerialSink *sink, const SerialRsyncIndexElt* obj) {

  return serialize_rsyncindexelt (sink, obj->md5, &obj->cksum);
}

/* RsyncIndexElt Unserialize
 */

gboolean
unserialize_rsyncindexelt_internal_noalloc (SerialSource *source, SerialRsyncIndexElt* result)
{
  if (! (* source->next_bytes_known) (source, result->md5, 16)) goto bail;
  if (! unserialize_xdeltachecksum_internal_noalloc (source, &result->cksum)) goto bail;
  return TRUE;
bail:
  return FALSE;
}

gboolean
unserialize_rsyncindexelt_internal (SerialSource *source, SerialRsyncIndexElt** result)
{
  SerialRsyncIndexElt* unser;
  (*result) = NULL;
  unser = serializeio_source_alloc (source, sizeof (SerialRsyncIndexElt));
  if (! unser) goto bail;
  if (! unserialize_rsyncindexelt_internal_noalloc (source, unser)) goto bail;
  (*result) = unser;
  return TRUE;
bail:
  return FALSE;
}

gboolean
unserialize_rsyncindexelt (SerialSource *source, SerialRsyncIndexElt** result)
{
  if ( (* source->source_type) (source, TRUE) != ST_RsyncIndexElt) goto bail;
  if (! unserialize_rsyncindexelt_internal (source, result)) goto bail;
  if (! serializeio_source_object_received (source)) goto bail;
  return TRUE;
bail:
  return FALSE;
}

/* RsyncIndex Count
 */

guint
serializeio_count_rsyncindex (guint32 seg_len, guint32 file_len, const guint8 file_md5[16], guint32 index_len, SerialRsyncIndexElt const* index) {
  guint size = sizeof (SerialRsyncIndex);
  ALIGN_8 (size);
  ALIGN_8 (size);
  ALIGN_8 (size);
  ALIGN_8 (size);
  {
    gint i;
    for (i = 0; i < index_len; i += 1)
      {
        size += serializeio_count_rsyncindexelt_obj (& (index[i]));
      }
  }
  ALIGN_8 (size);
  return size;
}

guint
serializeio_count_rsyncindex_obj (SerialRsyncIndex const* obj) {
  return serializeio_count_rsyncindex (obj->seg_len, obj->file_len, obj->file_md5, obj->index_len, obj->index);
}

/* RsyncIndex Print
 */

void
serializeio_print_rsyncindex_obj (SerialRsyncIndex* obj, guint indent_spaces) {
  print_spaces (indent_spaces);
  g_print ("[ST_RsyncIndex]\n");
  print_spaces (indent_spaces);
  g_print ("seg_len = ");
  g_print ("%d\n", obj->seg_len);
  print_spaces (indent_spaces);
  g_print ("file_len = ");
  g_print ("%d\n", obj->file_len);
  print_spaces (indent_spaces);
  g_print ("file_md5 = ");
  serializeio_print_bytes (obj->file_md5, 16);
  print_spaces (indent_spaces);
  g_print ("index = ");
  g_print ("{\n");
  {
    gint i;
    for (i = 0; i < obj->index_len; i += 1)
      {
        print_spaces (indent_spaces);
        g_print ("%d: ", i);
        print_spaces (indent_spaces);
      serializeio_print_rsyncindexelt_obj (& (obj->index[i]), indent_spaces + 2);
      print_spaces (indent_spaces);
;
      }
  }
  g_print ("}\n");
}

/* RsyncIndex Serialize
 */

gboolean
serialize_rsyncindex_internal (SerialSink *sink, guint32 seg_len, guint32 file_len, const guint8 file_md5[16], guint32 index_len, SerialRsyncIndexElt const* index)
{
  if (! (* sink->next_uint) (sink, seg_len)) goto bail;
  if (! (* sink->next_uint) (sink, file_len)) goto bail;
  if (! (* sink->next_bytes_known) (sink, file_md5, 16)) goto bail;
  {
    gint i;
    if (! (* sink->next_uint) (sink, index_len)) goto bail;
    for (i = 0; i < index_len; i += 1)
      {
        if (! serialize_rsyncindexelt_internal (sink, (index[i]).md5, &(index[i]).cksum)) goto bail;
      }
  }
  return TRUE;
bail:
  return FALSE;
}

gboolean
serialize_rsyncindex_obj_internal (SerialSink *sink, SerialRsyncIndex* obj)
{
  return serialize_rsyncindex_internal (sink, obj->seg_len, obj->file_len, obj->file_md5, obj->index_len, obj->index);
}

gboolean
serialize_rsyncindex (SerialSink *sink, guint32 seg_len, guint32 file_len, const guint8 file_md5[16], guint32 index_len, SerialRsyncIndexElt const* index)
{
  if (! (* sink->sink_type) (sink, ST_RsyncIndex, serializeio_count_rsyncindex (seg_len, file_len, file_md5, index_len, index), TRUE)) goto bail;
  if (! serialize_rsyncindex_internal (sink, seg_len, file_len, file_md5, index_len, index)) goto bail;
  if (sink->sink_quantum && ! sink->sink_quantum (sink)) goto bail;
  return TRUE;
bail:
  return FALSE;
}

gboolean
serialize_rsyncindex_obj (SerialSink *sink, const SerialRsyncIndex* obj) {

  return serialize_rsyncindex (sink, obj->seg_len, obj->file_len, obj->file_md5, obj->index_len, obj->index);
}

/* RsyncIndex Unserialize
 */

gboolean
unserialize_rsyncindex_internal_noalloc (SerialSource *source, SerialRsyncIndex* result)
{
  if (! (* source->next_uint) (source, &result->seg_len)) goto bail;
  if (! (* source->next_uint) (source, &result->file_len)) goto bail;
  if (! (* source->next_bytes_known) (source, result->file_md5, 16)) goto bail;
  {
    gint i;
    if (! (* source->next_uint) (source, &result->index_len)) goto bail;
    if (! (result->index = serializeio_source_alloc (source, sizeof (SerialRsyncIndexElt) * result->index_len))) goto bail;
    for (i = 0; i < result->index_len; i += 1)
      {
        if (! unserialize_rsyncindexelt_internal_noalloc (source, &(result->index[i]))) goto bail;
      }
  }
  return TRUE;
bail:
  return FALSE;
}

gboolean
unserialize_rsyncindex_internal (SerialSource *source, SerialRsyncIndex** result)
{
  SerialRsyncIndex* unser;
  (*result) = NULL;
  unser = serializeio_source_alloc (source, sizeof (SerialRsyncIndex));
  if (! unser) goto bail;
  if (! unserialize_rsyncindex_internal_noalloc (source, unser)) goto bail;
  (*result) = unser;
  return TRUE;
bail:
  return FALSE;
}

gboolean
unserialize_rsyncindex (SerialSource *source, SerialRsyncIndex** result)
{
  if ( (* source->source_type) (source, TRUE) != ST_RsyncIndex) goto bail;
  if (! unserialize_rsyncindex_internal (source, result)) goto bail;
  if (! serializeio_source_object_received (source)) goto bail;
  return TRUE;
bail:
  return FALSE;
}

/* Version0SourceInfo Count
 */

guint
serializeio_count_version0sourceinfo (const guint8 md5[16], const guint8 real_md5[16], guint32 length) {
  guint size = sizeof (SerialVersion0SourceInfo);
  ALIGN_8 (size);
  ALIGN_8 (size);
  ALIGN_8 (size);
  ALIGN_8 (size);
  return size;
}

guint
serializeio_count_version0sourceinfo_obj (SerialVersion0SourceInfo const* obj) {
  return serializeio_count_version0sourceinfo (obj->md5, obj->real_md5, obj->length);
}

/* Version0SourceInfo Print
 */

void
serializeio_print_version0sourceinfo_obj (SerialVersion0SourceInfo* obj, guint indent_spaces) {
  print_spaces (indent_spaces);
  g_print ("[ST_Version0SourceInfo]\n");
  print_spaces (indent_spaces);
  g_print ("md5 = ");
  serializeio_print_bytes (obj->md5, 16);
  print_spaces (indent_spaces);
  g_print ("real_md5 = ");
  serializeio_print_bytes (obj->real_md5, 16);
  print_spaces (indent_spaces);
  g_print ("length = ");
  g_print ("%d\n", obj->length);
}

/* Version0SourceInfo Serialize
 */

gboolean
serialize_version0sourceinfo_internal (SerialSink *sink, const guint8 md5[16], const guint8 real_md5[16], guint32 length)
{
  if (! (* sink->next_bytes_known) (sink, md5, 16)) goto bail;
  if (! (* sink->next_bytes_known) (sink, real_md5, 16)) goto bail;
  if (! (* sink->next_uint) (sink, length)) goto bail;
  return TRUE;
bail:
  return FALSE;
}

gboolean
serialize_version0sourceinfo_obj_internal (SerialSink *sink, SerialVersion0SourceInfo* obj)
{
  return serialize_version0sourceinfo_internal (sink, obj->md5, obj->real_md5, obj->length);
}

gboolean
serialize_version0sourceinfo (SerialSink *sink, const guint8 md5[16], const guint8 real_md5[16], guint32 length)
{
  if (! (* sink->sink_type) (sink, ST_Version0SourceInfo, serializeio_count_version0sourceinfo (md5, real_md5, length), TRUE)) goto bail;
  if (! serialize_version0sourceinfo_internal (sink, md5, real_md5, length)) goto bail;
  if (sink->sink_quantum && ! sink->sink_quantum (sink)) goto bail;
  return TRUE;
bail:
  return FALSE;
}

gboolean
serialize_version0sourceinfo_obj (SerialSink *sink, const SerialVersion0SourceInfo* obj) {

  return serialize_version0sourceinfo (sink, obj->md5, obj->real_md5, obj->length);
}

/* Version0SourceInfo Unserialize
 */

gboolean
unserialize_version0sourceinfo_internal_noalloc (SerialSource *source, SerialVersion0SourceInfo* result)
{
  if (! (* source->next_bytes_known) (source, result->md5, 16)) goto bail;
  if (! (* source->next_bytes_known) (source, result->real_md5, 16)) goto bail;
  if (! (* source->next_uint) (source, &result->length)) goto bail;
  return TRUE;
bail:
  return FALSE;
}

gboolean
unserialize_version0sourceinfo_internal (SerialSource *source, SerialVersion0SourceInfo** result)
{
  SerialVersion0SourceInfo* unser;
  (*result) = NULL;
  unser = serializeio_source_alloc (source, sizeof (SerialVersion0SourceInfo));
  if (! unser) goto bail;
  if (! unserialize_version0sourceinfo_internal_noalloc (source, unser)) goto bail;
  (*result) = unser;
  return TRUE;
bail:
  return FALSE;
}

gboolean
unserialize_version0sourceinfo (SerialSource *source, SerialVersion0SourceInfo** result)
{
  if ( (* source->source_type) (source, TRUE) != ST_Version0SourceInfo) goto bail;
  if (! unserialize_version0sourceinfo_internal (source, result)) goto bail;
  if (! serializeio_source_object_received (source)) goto bail;
  return TRUE;
bail:
  return FALSE;
}

/* Version0Control Count
 */

guint
serializeio_count_version0control (gboolean normalized, guint32 data_len, SerialVersion0SourceInfo const* to_info, guint32 source_info_len, SerialVersion0SourceInfo* const* source_info, guint32 inst_len, SerialVersion0Instruction const* inst) {
  guint size = sizeof (SerialVersion0Control);
  ALIGN_8 (size);
  ALIGN_8 (size);
  ALIGN_8 (size);
  size += serializeio_count_version0sourceinfo_obj (to_info) - sizeof (SerialVersion0SourceInfo);
  ALIGN_8 (size);
  {
    gint i;
    for (i = 0; i < source_info_len; i += 1)
      {
        size += serializeio_count_version0sourceinfo_obj ((source_info[i])) + sizeof (void*);
      }
  }
  ALIGN_8 (size);
  {
    gint i;
    for (i = 0; i < inst_len; i += 1)
      {
        size += serializeio_count_version0instruction_obj (& (inst[i]));
      }
  }
  ALIGN_8 (size);
  return size;
}

guint
serializeio_count_version0control_obj (SerialVersion0Control const* obj) {
  return serializeio_count_version0control (obj->normalized, obj->data_len, &obj->to_info, obj->source_info_len, obj->source_info, obj->inst_len, obj->inst);
}

/* Version0Control Print
 */

void
serializeio_print_version0control_obj (SerialVersion0Control* obj, guint indent_spaces) {
  print_spaces (indent_spaces);
  g_print ("[ST_Version0Control]\n");
  print_spaces (indent_spaces);
  g_print ("normalized = ");
  g_print ("%s\n", obj->normalized ? "true" : "false");
  print_spaces (indent_spaces);
  g_print ("data_len = ");
  g_print ("%d\n", obj->data_len);
  print_spaces (indent_spaces);
  g_print ("to_info = ");
  g_print ("{\n");
  serializeio_print_version0sourceinfo_obj (& obj->to_info, indent_spaces + 2);
  print_spaces (indent_spaces);
;
  g_print ("}\n");
  print_spaces (indent_spaces);
  g_print ("source_info = ");
  g_print ("{\n");
  {
    gint i;
    for (i = 0; i < obj->source_info_len; i += 1)
      {
        print_spaces (indent_spaces);
        g_print ("%d: ", i);
        print_spaces (indent_spaces);
      serializeio_print_version0sourceinfo_obj ((obj->source_info[i]), indent_spaces + 2);
      print_spaces (indent_spaces);
;
      }
  }
  g_print ("}\n");
  print_spaces (indent_spaces);
  g_print ("inst = ");
  g_print ("{\n");
  {
    gint i;
    for (i = 0; i < obj->inst_len; i += 1)
      {
        print_spaces (indent_spaces);
        g_print ("%d: ", i);
        print_spaces (indent_spaces);
      serializeio_print_version0instruction_obj (& (obj->inst[i]), indent_spaces + 2);
      print_spaces (indent_spaces);
;
      }
  }
  g_print ("}\n");
}

/* Version0Control Serialize
 */

gboolean
serialize_version0control_internal (SerialSink *sink, gboolean normalized, guint32 data_len, SerialVersion0SourceInfo const* to_info, guint32 source_info_len, SerialVersion0SourceInfo* const* source_info, guint32 inst_len, SerialVersion0Instruction const* inst)
{
  if (! (* sink->next_bool) (sink, normalized)) goto bail;
  if (! (* sink->next_uint) (sink, data_len)) goto bail;
  if (! serialize_version0sourceinfo_internal (sink, to_info->md5, to_info->real_md5, to_info->length)) goto bail;
  {
    gint i;
    if (! (* sink->next_uint) (sink, source_info_len)) goto bail;
    for (i = 0; i < source_info_len; i += 1)
      {
        if (! serialize_version0sourceinfo_internal (sink, (source_info[i])->md5, (source_info[i])->real_md5, (source_info[i])->length)) goto bail;
      }
  }
  {
    gint i;
    if (! (* sink->next_uint) (sink, inst_len)) goto bail;
    for (i = 0; i < inst_len; i += 1)
      {
        if (! serialize_version0instruction_internal (sink, (inst[i]).offset, (inst[i]).length)) goto bail;
      }
  }
  return TRUE;
bail:
  return FALSE;
}

gboolean
serialize_version0control_obj_internal (SerialSink *sink, SerialVersion0Control* obj)
{
  return serialize_version0control_internal (sink, obj->normalized, obj->data_len, &obj->to_info, obj->source_info_len, obj->source_info, obj->inst_len, obj->inst);
}

gboolean
serialize_version0control (SerialSink *sink, gboolean normalized, guint32 data_len, SerialVersion0SourceInfo const* to_info, guint32 source_info_len, SerialVersion0SourceInfo* const* source_info, guint32 inst_len, SerialVersion0Instruction const* inst)
{
  if (! (* sink->sink_type) (sink, ST_Version0Control, serializeio_count_version0control (normalized, data_len, to_info, source_info_len, source_info, inst_len, inst), TRUE)) goto bail;
  if (! serialize_version0control_internal (sink, normalized, data_len, to_info, source_info_len, source_info, inst_len, inst)) goto bail;
  if (sink->sink_quantum && ! sink->sink_quantum (sink)) goto bail;
  return TRUE;
bail:
  return FALSE;
}

gboolean
serialize_version0control_obj (SerialSink *sink, const SerialVersion0Control* obj) {

  return serialize_version0control (sink, obj->normalized, obj->data_len, &obj->to_info, obj->source_info_len, obj->source_info, obj->inst_len, obj->inst);
}

/* Version0Control Unserialize
 */

gboolean
unserialize_version0control_internal_noalloc (SerialSource *source, SerialVersion0Control* result)
{
  if (! (* source->next_bool) (source, &result->normalized)) goto bail;
  if (! (* source->next_uint) (source, &result->data_len)) goto bail;
  if (! unserialize_version0sourceinfo_internal_noalloc (source, &result->to_info)) goto bail;
  {
    gint i;
    if (! (* source->next_uint) (source, &result->source_info_len)) goto bail;
    if (! (result->source_info = serializeio_source_alloc (source, sizeof (SerialVersion0SourceInfo*) * result->source_info_len))) goto bail;
    for (i = 0; i < result->source_info_len; i += 1)
      {
        if (! unserialize_version0sourceinfo_internal (source, &(result->source_info[i]))) goto bail;
      }
  }
  {
    gint i;
    if (! (* source->next_uint) (source, &result->inst_len)) goto bail;
    if (! (result->inst = serializeio_source_alloc (source, sizeof (SerialVersion0Instruction) * result->inst_len))) goto bail;
    for (i = 0; i < result->inst_len; i += 1)
      {
        if (! unserialize_version0instruction_internal_noalloc (source, &(result->inst[i]))) goto bail;
      }
  }
  return TRUE;
bail:
  return FALSE;
}

gboolean
unserialize_version0control_internal (SerialSource *source, SerialVersion0Control** result)
{
  SerialVersion0Control* unser;
  (*result) = NULL;
  unser = serializeio_source_alloc (source, sizeof (SerialVersion0Control));
  if (! unser) goto bail;
  if (! unserialize_version0control_internal_noalloc (source, unser)) goto bail;
  (*result) = unser;
  return TRUE;
bail:
  return FALSE;
}

gboolean
unserialize_version0control (SerialSource *source, SerialVersion0Control** result)
{
  if ( (* source->source_type) (source, TRUE) != ST_Version0Control) goto bail;
  if (! unserialize_version0control_internal (source, result)) goto bail;
  if (! serializeio_source_object_received (source)) goto bail;
  return TRUE;
bail:
  return FALSE;
}

/* Version0Instruction Count
 */

guint
serializeio_count_version0instruction (guint32 offset, guint32 length) {
  guint size = sizeof (SerialVersion0Instruction);
  ALIGN_8 (size);
  ALIGN_8 (size);
  ALIGN_8 (size);
  return size;
}

guint
serializeio_count_version0instruction_obj (SerialVersion0Instruction const* obj) {
  return serializeio_count_version0instruction (obj->offset, obj->length);
}

/* Version0Instruction Print
 */

void
serializeio_print_version0instruction_obj (SerialVersion0Instruction* obj, guint indent_spaces) {
  print_spaces (indent_spaces);
  g_print ("[ST_Version0Instruction]\n");
  print_spaces (indent_spaces);
  g_print ("offset = ");
  g_print ("%d\n", obj->offset);
  print_spaces (indent_spaces);
  g_print ("length = ");
  g_print ("%d\n", obj->length);
}

/* Version0Instruction Serialize
 */

gboolean
serialize_version0instruction_internal (SerialSink *sink, guint32 offset, guint32 length)
{
  if (! (* sink->next_uint) (sink, offset)) goto bail;
  if (! (* sink->next_uint) (sink, length)) goto bail;
  return TRUE;
bail:
  return FALSE;
}

gboolean
serialize_version0instruction_obj_internal (SerialSink *sink, SerialVersion0Instruction* obj)
{
  return serialize_version0instruction_internal (sink, obj->offset, obj->length);
}

gboolean
serialize_version0instruction (SerialSink *sink, guint32 offset, guint32 length)
{
  if (! (* sink->sink_type) (sink, ST_Version0Instruction, serializeio_count_version0instruction (offset, length), TRUE)) goto bail;
  if (! serialize_version0instruction_internal (sink, offset, length)) goto bail;
  if (sink->sink_quantum && ! sink->sink_quantum (sink)) goto bail;
  return TRUE;
bail:
  return FALSE;
}

gboolean
serialize_version0instruction_obj (SerialSink *sink, const SerialVersion0Instruction* obj) {

  return serialize_version0instruction (sink, obj->offset, obj->length);
}

/* Version0Instruction Unserialize
 */

gboolean
unserialize_version0instruction_internal_noalloc (SerialSource *source, SerialVersion0Instruction* result)
{
  if (! (* source->next_uint) (source, &result->offset)) goto bail;
  if (! (* source->next_uint) (source, &result->length)) goto bail;
  return TRUE;
bail:
  return FALSE;
}

gboolean
unserialize_version0instruction_internal (SerialSource *source, SerialVersion0Instruction** result)
{
  SerialVersion0Instruction* unser;
  (*result) = NULL;
  unser = serializeio_source_alloc (source, sizeof (SerialVersion0Instruction));
  if (! unser) goto bail;
  if (! unserialize_version0instruction_internal_noalloc (source, unser)) goto bail;
  (*result) = unser;
  return TRUE;
bail:
  return FALSE;
}

gboolean
unserialize_version0instruction (SerialSource *source, SerialVersion0Instruction** result)
{
  if ( (* source->source_type) (source, TRUE) != ST_Version0Instruction) goto bail;
  if (! unserialize_version0instruction_internal (source, result)) goto bail;
  if (! serializeio_source_object_received (source)) goto bail;
  return TRUE;
bail:
  return FALSE;
}

void
xd_generate_void_event_internal (XdVoidEventCode _code, const char* _srcfile, gint _srcline)
{
  XdVoidEvent *_e = g_new0 (XdVoidEvent, 1);
  _e->code = _code.code;
  _e->srcline = _srcline;
  _e->srcfile = _srcfile;
  eventdelivery_event_deliver ((GenericEvent*) _e);
}

const XdVoidEventCode EC_XdTooFewSources = { EC_XdTooFewSourcesValue };

const XdVoidEventCode EC_XdTooManySources = { EC_XdTooManySourcesValue };

void
xd_generate_int_event_internal (XdIntEventCode _code, const char* _srcfile, gint _srcline, int index)
{
  XdIntEvent *_e = g_new0 (XdIntEvent, 1);
  _e->code = _code.code;
  _e->srcline = _srcline;
  _e->srcfile = _srcfile;
  _e->index = index;
  eventdelivery_event_deliver ((GenericEvent*) _e);
}

const char*
Xd_Int_event_field_to_string (GenericEvent* ev, gint field)
{
  XdIntEvent* it = (XdIntEvent*) ev;
  switch (field)
    {
    case 0: return eventdelivery_int_to_string (it->index);
    default: abort ();
    }
}

const XdIntEventCode EC_XdOutOfRangeSourceIndex = { EC_XdOutOfRangeSourceIndexValue };

const XdVoidEventCode EC_XdInvalidControl = { EC_XdInvalidControlValue };

const XdVoidEventCode EC_XdInvalidRsyncCache = { EC_XdInvalidRsyncCacheValue };

const XdVoidEventCode EC_XdIncompatibleDelta = { EC_XdIncompatibleDeltaValue };

void
xd_generate_handlestringstring_event_internal (XdHandleStringStringEventCode _code, const char* _srcfile, gint _srcline, FileHandle* stream, const char* expected, const char* received)
{
  XdHandleStringStringEvent *_e = g_new0 (XdHandleStringStringEvent, 1);
  _e->code = _code.code;
  _e->srcline = _srcline;
  _e->srcfile = _srcfile;
  _e->stream = stream;
  _e->expected = expected;
  _e->received = received;
  eventdelivery_event_deliver ((GenericEvent*) _e);
}

const char*
Xd_HandleStringString_event_field_to_string (GenericEvent* ev, gint field)
{
  XdHandleStringStringEvent* it = (XdHandleStringStringEvent*) ev;
  switch (field)
    {
    case 0: return eventdelivery_handle_to_string (it->stream);
    case 1: return eventdelivery_string_to_string (it->expected);
    case 2: return eventdelivery_string_to_string (it->received);
    default: abort ();
    }
}

const XdHandleStringStringEventCode EC_XdStreamChecksumFailed = { EC_XdStreamChecksumFailedValue };

void
xd_generate_handleintint_event_internal (XdHandleIntIntEventCode _code, const char* _srcfile, gint _srcline, FileHandle* stream, int expected, int received)
{
  XdHandleIntIntEvent *_e = g_new0 (XdHandleIntIntEvent, 1);
  _e->code = _code.code;
  _e->srcline = _srcline;
  _e->srcfile = _srcfile;
  _e->stream = stream;
  _e->expected = expected;
  _e->received = received;
  eventdelivery_event_deliver ((GenericEvent*) _e);
}

const char*
Xd_HandleIntInt_event_field_to_string (GenericEvent* ev, gint field)
{
  XdHandleIntIntEvent* it = (XdHandleIntIntEvent*) ev;
  switch (field)
    {
    case 0: return eventdelivery_handle_to_string (it->stream);
    case 1: return eventdelivery_int_to_string (it->expected);
    case 2: return eventdelivery_int_to_string (it->received);
    default: abort ();
    }
}

const XdHandleIntIntEventCode EC_XdStreamLengthFailed = { EC_XdStreamLengthFailedValue };

void
xd_generate_string_event_internal (XdStringEventCode _code, const char* _srcfile, gint _srcline, const char* version)
{
  XdStringEvent *_e = g_new0 (XdStringEvent, 1);
  _e->code = _code.code;
  _e->srcline = _srcline;
  _e->srcfile = _srcfile;
  _e->version = version;
  eventdelivery_event_deliver ((GenericEvent*) _e);
}

const char*
Xd_String_event_field_to_string (GenericEvent* ev, gint field)
{
  XdStringEvent* it = (XdStringEvent*) ev;
  switch (field)
    {
    case 0: return eventdelivery_string_to_string (it->version);
    default: abort ();
    }
}

const XdStringEventCode EC_XdBackwardCompatibilityMode = { EC_XdBackwardCompatibilityModeValue };

