/* -*-Mode: C;-*-
 * $Id: edsiotest.c 1.6 Sat, 03 Apr 1999 03:23:54 -0800 jmacd $
 *
 * Copyright (C) 1998, 1999, Josh MacDonald.
 * All Rights Reserved.
 *
 * Author: Josh MacDonald <jmacd@CS.Berkeley.EDU>
 */

#include "edsio.h"

void
test1 ()
{
  guint32 x = 0;

  PropTest *pt = g_new0 (PropTest, 1);

  EdsioPropTestUintProperty prop;

  g_assert (edsio_edsio_init ());

  g_assert (edsio_new_proptest_uint_property ("Just testing (1)...", PF_Persistent, & prop));

  g_assert (! proptest_isset_uint (pt, prop));

  g_assert (proptest_set_uint (pt, prop, 42));

  g_assert (proptest_isset_uint (pt, prop));

  g_assert (proptest_get_uint (pt, prop, & x) && x == 42);

  /* kill the cache, to test persistence. */
  pt->_edsio_property_table = NULL;

  g_assert (proptest_get_uint (pt, prop, & x) && x == 42);

  g_assert (proptest_unset_uint (pt, prop));

  g_assert (! proptest_isset_uint (pt, prop));

  g_assert (! pt->_edsio_property_table);
}

void
test2 ()
{
  const char* str = "hello there";
  const char* str2;
  guint32 str2_len;

  PropTest *pt = g_new0 (PropTest, 1);

  EdsioPropTestBytesProperty prop;

  g_assert (edsio_edsio_init ());

  g_assert (edsio_new_proptest_bytes_property ("Just testing (2)...", PF_Persistent, & prop));

  g_assert (! proptest_isset_bytes (pt, prop));

  g_assert (proptest_set_bytes (pt, prop, (guint8*) str, strlen (str) + 1));

  g_assert (proptest_isset_bytes (pt, prop));

  g_assert (proptest_get_bytes (pt, prop, (const guint8**) & str2, & str2_len) && str2_len == (strlen (str) + 1) && strcmp (str, str2) == 0);

  /* kill the cache, to test persistence. */
  pt->_edsio_property_table = NULL;

  g_assert (proptest_get_bytes (pt, prop, (const guint8**) & str2, & str2_len) && str2_len == (strlen (str) + 1) && strcmp (str, str2) == 0);

  g_assert (proptest_unset_bytes (pt, prop));

  g_assert (! proptest_isset_bytes (pt, prop));

  g_assert (! pt->_edsio_property_table);
}

void
test3 ()
{
  SerialEdsioUint x;
  SerialEdsioUint *p;
  EdsioPropTestEdsioUintProperty prop;

  PropTest *pt = g_new0 (PropTest, 1);

  x.val = 42;

  g_assert (edsio_edsio_init ());

  g_assert (edsio_new_proptest_edsiouint_property ("Just testing (3)...", PF_Persistent, & prop));

  g_assert (! proptest_isset_edsiouint (pt, prop));

  g_assert (proptest_set_edsiouint (pt, prop, & x));

  g_assert (proptest_isset_edsiouint (pt, prop));

  g_assert (proptest_get_edsiouint (pt, prop, & p) && x.val == p->val);

  /* kill the cache, to test persistence. */
  pt->_edsio_property_table = NULL;

  g_assert (proptest_get_edsiouint (pt, prop, & p) && x.val == p->val);

  g_assert (proptest_unset_edsiouint (pt, prop));

  g_assert (! proptest_isset_edsiouint (pt, prop));

  g_assert (! pt->_edsio_property_table);
}

void
test4 ()
{
  const char* str = "hello there";
  const char* str2;

  PropTest *pt = g_new0 (PropTest, 1);

  EdsioPropTestStringProperty prop;

  g_assert (edsio_edsio_init ());

  g_assert (edsio_new_proptest_string_property ("Just testing (4)...", PF_Persistent, & prop));

  g_assert (! proptest_isset_string (pt, prop));

  g_assert (proptest_set_string (pt, prop, str));

  g_assert (proptest_isset_string (pt, prop));

  g_assert (proptest_get_string (pt, prop, & str2) && strcmp (str, str2) == 0);

  /* kill the cache, to test persistence. */
  pt->_edsio_property_table = NULL;

  g_assert (proptest_get_string (pt, prop, & str2) && strcmp (str, str2) == 0);

  g_assert (proptest_unset_string (pt, prop));

  g_assert (! proptest_isset_string (pt, prop));

  g_assert (! pt->_edsio_property_table);
}

void test5 ()
{
  GByteArray* sink_result;
  SerialSink* sink = edsio_simple_sink (NULL, SBF_Checksum | SBF_Base64 | SBF_Compress, FALSE, NULL, & sink_result);
  SerialSource* src;
  const char* input = "hello there!!!!!!!!";
  SerialEdsioString *output;
  guint8 zero[1];
  zero[0] = 0;

  g_assert (serialize_edsiostring (sink, input));

  g_assert (sink->sink_close (sink));

  g_byte_array_append (sink_result, zero, 1);

  g_print ("%s -> %s\n", input, sink_result->data);

  src = edsio_simple_source (sink_result->data, sink_result->len - 1, SBF_Checksum | SBF_Base64 | SBF_Compress);

  g_assert (unserialize_edsiostring (src, & output));

  g_assert (src->source_close (src));

  g_assert (strcmp (output->val, input) == 0);
}

void
test6 ()
{
  const char* md5str = "aed3918c4ccb89f2dcf74d5dcab22989";
  const char* md5strbad1 = "aed3918c4cXb89f2dcf74d5dcab22989";
  const char* md5strbad2 = "aed3918c4cab89f2dcf74d5dcab22989X";
  const char* md5strbad3 = "aed3918c4cab89f2dcf74d5dcab2298";
  char md5str2[33];
  guint8 md5[16];

  g_assert (! edsio_md5_from_string (md5, md5strbad1));
  g_assert (! edsio_md5_from_string (md5, md5strbad2));
  g_assert (! edsio_md5_from_string (md5, md5strbad3));

  g_assert (edsio_md5_from_string (md5, md5str));

  edsio_md5_to_string (md5, md5str2);

  g_assert (strcmp (md5str, md5str2) == 0);
}

int
main ()
{
  test1 ();

  test2 ();

  test3 ();

  test4 ();

  test5 ();

  test6 ();

  return 0;
}
