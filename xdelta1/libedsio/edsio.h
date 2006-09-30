/* -*-Mode: C;-*-
 * $Id: edsio.h 1.15.1.1 Mon, 11 Jun 2001 01:56:01 -0700 jmacd $
 *
 * Copyright (C) 1998, 1999, Josh MacDonald.
 * All Rights Reserved.
 *
 * Author: Josh MacDonald <jmacd@CS.Berkeley.EDU>
 */

#ifndef _EDSIO_H_
#define _EDSIO_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <glib.h>

typedef struct _SerialSource    SerialSource;
typedef struct _SerialSink      SerialSink;
typedef gint32                  SerialType;
typedef struct _HandleFuncTable HandleFuncTable;
typedef struct _PropTest        PropTest;
typedef struct _FileHandle      FileHandle;

struct _FileHandle {
  const HandleFuncTable* table;

  /* This is an opaque type, feel free to define your own, just make
   * sure that the first field of yours is one of these, and that you
   * supply a table. */
};

#define EDSIO_LIBRARY_OFFSET_BITS 8
#define EDSIO_LIBRARY_OFFSET_MASK 0xff

#include "edsio_edsio.h"

#define ST_Error         -1
#define ST_IncorrectType -2
#define ST_NotFound      -3

#define ALIGN_8(v) if (((v) % 8) != 0) { (v) += 8; (v) &= ~7; }

/* This serves as a virtual table for I/O to the FileHandle */

struct _HandleFuncTable
{
  gssize            (* table_handle_length)       (FileHandle *fh);
  gssize            (* table_handle_pages)        (FileHandle *fh);
  gssize            (* table_handle_pagesize)     (FileHandle *fh);
  gssize            (* table_handle_map_page)     (FileHandle *fh, guint pgno, const guint8** mem);
  gboolean          (* table_handle_unmap_page)   (FileHandle *fh, guint pgno, const guint8** mem);
  const guint8*     (* table_handle_checksum_md5) (FileHandle *fh);
  gboolean          (* table_handle_close)        (FileHandle *fh, gint flags);
  gboolean          (* table_handle_write)        (FileHandle *fh, const guint8 *buf, gsize nbyte);
  gboolean          (* table_handle_copy)         (FileHandle *from, FileHandle *to, guint off, guint len);
  gboolean          (* table_handle_getui)        (FileHandle *fh, guint32* i);
  gboolean          (* table_handle_putui)        (FileHandle *fh, guint32 i);
  gssize            (* table_handle_read)         (FileHandle *fh, guint8 *buf, gsize nbyte);
  const gchar*      (* table_handle_name)         (FileHandle *fh); /* user must free */
};

struct _SerialSource {
  /* Internal variables: don't touch. */
  guint32  alloc_total;
  guint32  alloc_pos;
  void    *alloc_buf;
  void    *alloc_buf_orig;

  /* These are setup by init.
   */
  SerialType (* source_type)           (SerialSource* source, gboolean set_allocation);
  gboolean   (* source_close)          (SerialSource* source);
  gboolean   (* source_read)           (SerialSource* source, guint8 *ptr, guint32 len);
  void       (* source_free)           (SerialSource* source);

  /* These may be NULL
   */
  void*  (* salloc_func)           (SerialSource* source,
				    guint32       len);
  void   (* sfree_func)            (SerialSource* source,
				    void*         ptr);

  /* Public functions, defaulted, but may be over-ridden
   * before calls to unserialize.
   */
  gboolean   (* next_bytes_known)    (SerialSource* source, guint8        *ptr, guint32  len);
  gboolean   (* next_bytes)          (SerialSource* source, const guint8 **ptr, guint32 *len);
  gboolean   (* next_uint)           (SerialSource* source, guint32       *ptr);
  gboolean   (* next_uint32)         (SerialSource* source, guint32       *ptr);
  gboolean   (* next_uint16)         (SerialSource* source, guint16       *ptr);
  gboolean   (* next_uint8)          (SerialSource* source, guint8        *ptr);
  gboolean   (* next_bool)           (SerialSource* source, gboolean      *ptr);
  gboolean   (* next_string)         (SerialSource* source, const char   **ptr);
};

struct _SerialSink {

  /* These are setup by init.
   */
  gboolean     (* sink_type)          (SerialSink* sink, SerialType type, guint mem_size, gboolean set_allocation);
  gboolean     (* sink_close)         (SerialSink* sink);
  gboolean     (* sink_write)         (SerialSink* sink, const guint8 *ptr, guint32 len);
  void         (* sink_free)          (SerialSink* sink);

  /* This may be null, called after each object is serialized. */
  gboolean     (* sink_quantum)       (SerialSink* sink);

  /* Public functions, defaulted, but may be over-ridden
   * before calls to serialize.
   */
  gboolean   (* next_bytes_known)   (SerialSink* sink, const guint8 *ptr, guint32 len);
  gboolean   (* next_bytes)         (SerialSink* sink, const guint8 *ptr, guint32 len);
  gboolean   (* next_uint)          (SerialSink* sink, guint32       ptr);
  gboolean   (* next_uint32)        (SerialSink* sink, guint32       ptr);
  gboolean   (* next_uint16)        (SerialSink* sink, guint16       ptr);
  gboolean   (* next_uint8)         (SerialSink* sink, guint8        ptr);
  gboolean   (* next_bool)          (SerialSink* sink, gboolean      ptr);
  gboolean   (* next_string)        (SerialSink* sink, const char   *ptr);
};

void           serializeio_initialize_type                (const char* name,
							   guint32     val,
							   gboolean  (*unserialize_func) (),
							   gboolean  (*serialize_func) (),
							   guint     (*count_func) (),
							   void      (*print_func) ());

const char*    serializeio_generic_type_to_string         (SerialType type);
void           serializeio_generic_print                  (SerialType type, void* object, guint indent_spaces);

gboolean       serializeio_serialize_generic              (SerialSink    *sink,
							   SerialType     object_type,
							   void          *object);

gboolean       serializeio_serialize_generic_internal     (SerialSink    *sink,
							   SerialType     object_type,
							   void          *object,
							   gboolean       set_allocation);

guint          serializeio_generic_count                  (SerialType     object_type,
							   void          *object);

gboolean       serializeio_unserialize_generic            (SerialSource  *source,
							   SerialType    *object_type,
							   void         **object);

gboolean       serializeio_unserialize_generic_internal   (SerialSource  *source,
							   SerialType    *object_type,
							   void         **object,
							   gboolean       set_allocation);

gboolean       serializeio_unserialize_generic_acceptable (SerialSource*  source,
							   guint32        acceptable,
							   SerialType    *object_type,
							   void         **object);

void           serializeio_sink_init                      (SerialSink* sink,
							   gboolean (* sink_type) (SerialSink* sink,
										   SerialType type,
										   guint mem_size,
										   gboolean set_allocation),
							   gboolean (* sink_close) (SerialSink* sink),
							   gboolean (* sink_write) (SerialSink* sink,
										    const guint8 *ptr,
										    guint32 len),
							   void     (* sink_free) (SerialSink* sink),
							   gboolean (* sink_quantum) (SerialSink* sink));

void           serializeio_source_init                    (SerialSource* source,
							   SerialType (* source_type) (SerialSource* source,
										       gboolean set_allocation),
							   gboolean   (* source_close) (SerialSource* source),
							   gboolean   (* source_read) (SerialSource* source,
										       guint8 *ptr,
										       guint32 len),
							   void       (* source_free) (SerialSource* source),
							   void*      (* salloc_func) (SerialSource* source,
										       guint32       len),
							   void       (* sfree_func) (SerialSource* source,
										      void*         ptr));

/* These two functions are internal, don't use. */
gboolean       serializeio_source_object_received         (SerialSource* source);
void*          serializeio_source_alloc                   (SerialSource* source,
							   guint32       len);

SerialSink*    serializeio_gzip_sink   (SerialSink* sink);
SerialSource*  serializeio_gzip_source (SerialSource* source);

SerialSink*    serializeio_checksum_sink   (SerialSink* sink);
SerialSource*  serializeio_checksum_source (SerialSource* source);

SerialSink*    serializeio_base64_sink   (SerialSink* sink);
SerialSource*  serializeio_base64_source (SerialSource* source);

SerialSource*  handle_source (FileHandle *fh);
SerialSink*    handle_sink   (FileHandle *fh,
			      gpointer data1,
			      gpointer data2,
			      gpointer data3,
			      gboolean (* cont_onclose) (gpointer data1, gpointer data2, gpointer data3));

gboolean unserialize_uint   (SerialSource *source, guint32** x);
gboolean serialize_uint_obj (SerialSink *sink, guint32* x);

gboolean unserialize_string   (SerialSource *source, const char** x);
gboolean serialize_string_obj (SerialSink *sink, const char* x);

/* These are a bit odd, and edsio_property_bytes_{g,s}etter account for it.
 * Try not to use yourself. */
gboolean unserialize_bytes   (SerialSource *source, SerialEdsioBytes** x);
gboolean serialize_bytes_obj (SerialSink *sink, SerialEdsioBytes *x);

/* Event delivery
 */

enum _EventLevel
{
  EL_Information   = 1<<0,
  EL_Warning       = 1<<5,
  EL_Error         = 1<<10,
  EL_InternalError = 1<<15,
  EL_FatalError    = 1<<20
};

typedef enum _EventLevel EventLevel;

enum _EventFlags
{
  EF_None    = 1<<0,
  EF_OpenSSL = 1<<1
};

typedef enum _EventFlags EventFlags;

const char* eventdelivery_int_to_string     (int x);
const char* eventdelivery_string_to_string  (const char* x);
const char* eventdelivery_source_to_string  (SerialSource* x);
const char* eventdelivery_sink_to_string    (SerialSink* x);
const char* eventdelivery_handle_to_string  (FileHandle* x);

void serializeio_print_bytes (const guint8* buf, guint len);

/* Event delivery privates
 */

typedef struct _GenericEventDef GenericEventDef;
typedef struct _GenericEvent    GenericEvent;

struct _GenericEvent
{
  gint        code;
  const char* srcfile;
  guint       srcline;
};

GenericEventDef* eventdelivery_event_lookup (gint code);

void eventdelivery_event_deliver        (GenericEvent* e);

typedef gboolean (* ErrorDeliveryFunc) (GenericEvent* ev, GenericEventDef* def, const char* message);

void eventdelivery_event_watch_all (ErrorDeliveryFunc func);

void eventdelivery_initialize_event_def (gint        code,
					 gint        level,
					 gint        flags,
					 const char* name,
					 const char* oneline,
					 const char * (* field_to_string) (GenericEvent* ev, gint field));

const char* eventdelivery_ssl_errors_to_string (void);

struct _GenericEventDef
{
  gint        code;
  gint        level;
  gint        flags;
  const char *name;
  const char *oneline;

  const char * (* field_to_string) (GenericEvent* ev, gint field);
};

/* MD5.H - header file for MD5C.C */

/* Copyright (C) 1991-2, RSA Data Security, Inc. Created 1991. All
   rights reserved.

   License to copy and use this software is granted provided that it
   is identified as the "RSA Data Security, Inc. MD5 Message-Digest
   Algorithm" in all material mentioning or referencing this software
   or this function.

   License is also granted to make and use derivative works provided
   that such works are identified as "derived from the RSA Data
   Security, Inc. MD5 Message-Digest Algorithm" in all material
   mentioning or referencing the derived work.

   RSA Data Security, Inc. makes no representations concerning either
   the merchantability of this software or the suitability of this
   software for any particular purpose. It is provided "as is"
   without express or implied warranty of any kind.

   These notices must be retained in any copies of any part of this
   documentation and/or software.
   */

/* MD5 context. */
typedef struct {
    guint32 state[4]; /* state (ABCD) */
    guint32 count[2]; /* number of bits, modulo 2^64 (lsb first) */
    guint8 buffer[64]; /* input buffer */
} EdsioMD5Ctx;

void edsio_md5_init   (EdsioMD5Ctx *);
void edsio_md5_update (EdsioMD5Ctx *, const guint8 *, guint);
void edsio_md5_final  (guint8*, EdsioMD5Ctx *);

gint  edsio_md5_equal (gconstpointer   v,
		       gconstpointer   v2);
guint edsio_md5_hash  (gconstpointer   v);

void      edsio_md5_to_string   (const guint8* md5, char buf[33]);
gboolean  edsio_md5_from_string (guint8* md5,       const char buf[33]);

/* NIST Secure Hash Algorithm */
/* heavily modified from Peter C. Gutmann's implementation */
/* then taken from from Uwe Hollerbach, */
/* and then modified a little by Josh MacDonald. */

/* This code is in the public domain */

typedef struct {
    guint32 digest[5];             /* message digest */
    guint32 count_lo, count_hi;    /* 64-bit bit count */
    guint32 data[16];              /* SHA data buffer */
    int local;                  /* unprocessed amount in data */
} EdsioSHACtx;

void edsio_sha_init   (EdsioSHACtx *);
void edsio_sha_update (EdsioSHACtx *, const guint8 *, guint);
void edsio_sha_final  (guint8 *, EdsioSHACtx *);

gint  edsio_sha_equal (gconstpointer   v,
		       gconstpointer   v2);
guint edsio_sha_hash  (gconstpointer   v);

/* Misc stuff.
 */

/* These raise an error if errmsg is non-null.  The errmsg should
 * be something like "Invalid port number".  See edsio.ser for the
 * format.
 */
gboolean strtosi_checked (const char* str, gint32* i, const char* errmsg);
gboolean strtoss_checked (const char* str, gint16* s, const char* errmsg);

gboolean strtoui_checked (const char* str, guint32* i, const char* errmsg);
gboolean strtous_checked (const char* str, guint16* i, const char* errmsg);

const char* edsio_intern_string (const char* str);

GByteArray*    edsio_base64_encode_region           (const guint8* data, guint data_len);
GByteArray*    edsio_base64_decode_region           (const guint8* data, guint data_len);
gboolean       edsio_base64_encode_region_into      (const guint8* data, guint data_len, guint8* out, guint *out_len);
gboolean       edsio_base64_decode_region_into      (const guint8* data, guint data_len, guint8* out, guint *out_len);

gchar*   edsio_time_to_iso8601   (SerialGenericTime* time);
gchar*   edsio_time_t_to_iso8601 (GTime time);
gboolean edsio_time_of_day       (SerialGenericTime* time);

enum _SimpleBufferFlags {
  SBF_None     = 0,
  SBF_Compress = 1 << 0,
  SBF_Checksum = 1 << 1,
  SBF_Base64   = 1 << 2
};

typedef enum _SimpleBufferFlags SimpleBufferFlags;

SerialSource*  edsio_simple_source                  (const guint8* data, guint len, guint flags);
SerialSink*    edsio_simple_sink                    (gpointer data,
						     guint    flags,
						     gboolean free_result,
						     void (* success) (gpointer data, GByteArray* result),
						     GByteArray **result);

gboolean edsio_library_check                        (guint32 number);
void     edsio_library_register                     (guint32 number, const char*name);

/* (Persistent) Property stuff.
 */

enum _PropertyFlags {
  PF_None = 0,
  PF_Persistent = 1
};

typedef enum _PropertyFlags PropertyFlags;

typedef struct _EdsioProperty EdsioProperty;
typedef union _EdsioPropertyEntry EdsioPropertyEntry;
typedef struct _EdsioGenericProperty EdsioGenericProperty;

typedef void     (* PropFreeFunc) (gpointer obj);
typedef gboolean (* PropGSFunc) (/*gpointer obj, GHashTable** obj_table, EdsioProperty* prop, ... */);
typedef gboolean (* PropSerialize) (/*SerialSink* sink, ... */);
typedef gboolean (* PropUnserialize) (/*SerialSource* source, ... */);

typedef GHashTable**  (* PropertyTableFunc) (gpointer obj);
typedef SerialSource* (* PersistSourceFunc) (gpointer obj, const char* prop_name);
typedef SerialSink*   (* PersistSinkFunc)   (gpointer obj, const char* prop_name);
typedef gboolean      (* PersistIssetFunc)  (gpointer obj, const char* prop_name);
typedef gboolean      (* PersistUnsetFunc)  (gpointer obj, const char* prop_name);

void                 edsio_initialize_property_type (const char* t, PropFreeFunc freer, PropGSFunc getter, PropGSFunc setter, PropSerialize ser, PropUnserialize unser);
void                 edsio_initialize_host_type (const char*       ph,
						 PropertyTableFunc ptable,
						 PersistSourceFunc source,
						 PersistSinkFunc   sink,
						 PersistIssetFunc  isset,
						 PersistUnsetFunc  unset);

gboolean             edsio_property_isset  (const char* ph, const char* t, guint32 code, gpointer obj);
gboolean             edsio_property_unset  (const char* ph, const char* t, guint32 code, gpointer obj);

PropGSFunc           edsio_property_getter (const char* ph, const char* t, guint32 code, EdsioProperty** prop);
PropGSFunc           edsio_property_setter (const char* ph, const char* t, guint32 code, EdsioProperty** prop);

void                 edsio_property_uint_free   (gpointer obj);
gboolean             edsio_property_uint_getter (gpointer obj, EdsioProperty* ep, guint32* get);
gboolean             edsio_property_uint_setter (gpointer obj, EdsioProperty* ep, guint32  set);

void                 edsio_property_string_free   (gpointer obj);
gboolean             edsio_property_string_getter (gpointer obj, EdsioProperty* ep, const char** get);
gboolean             edsio_property_string_setter (gpointer obj, EdsioProperty* ep, const char*  set);

void                 edsio_property_bytes_free   (gpointer obj);
gboolean             edsio_property_bytes_getter (gpointer obj, EdsioProperty* ep, guint8** get, guint32 *get_len);
gboolean             edsio_property_bytes_setter (gpointer obj, EdsioProperty* ep, guint8* set, guint32 set_len);

void                 edsio_property_vptr_free   (gpointer obj);
gboolean             edsio_property_vptr_getter (gpointer obj, EdsioProperty* ep, void** get);
gboolean             edsio_property_vptr_setter (gpointer obj, EdsioProperty* ep, void* set);

EdsioPropertyEntry*  edsio_property_get (gpointer obj, EdsioProperty* ep);
gboolean             edsio_property_set (gpointer obj, EdsioProperty* ep, EdsioPropertyEntry* set);

gboolean             edsio_new_property (const char* name, const char* ph, const char* t, guint32 flags, EdsioGenericProperty* prop);

/* Testing...
 */

#define DEBUG_LIBEDSIO

#ifdef DEBUG_LIBEDSIO
struct _PropTest
{
  GHashTable* _edsio_property_table;

  GHashTable* ptable;

  const char* kludge;
};

GHashTable**  edsio_proptest_property_table (PropTest *pt);
SerialSource* edsio_persist_proptest_source (PropTest *pt, const char* prop_name);
SerialSink*   edsio_persist_proptest_sink   (PropTest *pt, const char* prop_name);
gboolean      edsio_persist_proptest_isset  (PropTest *pt, const char* prop_name);
gboolean      edsio_persist_proptest_unset  (PropTest *pt, const char* prop_name);

#endif

/* Missing glib stuff
 */

typedef struct _GQueue		GQueue;

struct _GQueue
{
  GList *list;
  GList *list_end;
  guint list_size;
};

/* Queues
 */

GQueue *	g_queue_new		(void);
void		g_queue_free		(GQueue *q);
guint		g_queue_get_size	(GQueue *q);
void		g_queue_push_front	(GQueue *q, gpointer data);
void		g_queue_push_back	(GQueue *q, gpointer data);
gpointer	g_queue_pop_front	(GQueue *q);
gpointer	g_queue_pop_back	(GQueue *q);

#define g_queue_empty(queue) \
	((((GQueue *)(queue)) && ((GQueue *)(queue))->list) ? FALSE : TRUE)

#define g_queue_peek_front(queue) \
	((((GQueue *)(queue)) && ((GQueue *)(queue))->list) ? \
		((GQueue *)(queue))->list->data : NULL)

#define g_queue_peek_back(queue) \
	((((GQueue *)(queue)) && ((GQueue *)(queue))->list_end) ? \
		((GQueue *)(queue))->list_end->data : NULL)

#define g_queue_index(queue,ptr) \
	((((GQueue *)(queue)) && ((GQueue *)(queue))->list) ? \
		g_list_index (((GQueue *)(queue))->list, (ptr)) : -1)

#define		g_queue_push		g_queue_push_back
#define		g_queue_pop		g_queue_pop_front
#define		g_queue_peek		g_queue_peek_front


#ifdef __cplusplus
}
#endif

#endif /* _EDSIO_H_ */
