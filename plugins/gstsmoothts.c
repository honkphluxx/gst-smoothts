/* GStreamer
 * Copyright (C) 2022 FIXME <fixme@example.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Suite 500,
 * Boston, MA 02110-1335, USA.
 */
/**
 * SECTION:element-gstsmoothts
 *
 * The smoothts element does FIXME stuff.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch-1.0 -v fakesrc ! smoothts ! FIXME ! fakesink
 * ]|
 * FIXME Describe what the pipeline does.
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>
#include <gst/base/gstbasetransform.h>
#include "gstsmoothts.h"

GST_DEBUG_CATEGORY_STATIC (gst_smoothts_debug_category);
#define GST_CAT_DEFAULT gst_smoothts_debug_category

/* prototypes */


static void gst_smoothts_set_property (GObject * object,
    guint property_id, const GValue * value, GParamSpec * pspec);
static void gst_smoothts_get_property (GObject * object,
    guint property_id, GValue * value, GParamSpec * pspec);
static void gst_smoothts_dispose (GObject * object);
static void gst_smoothts_finalize (GObject * object);

static gboolean gst_smoothts_accept_caps (GstBaseTransform * trans,
    GstPadDirection direction, GstCaps * caps);
static gboolean gst_smoothts_query (GstBaseTransform * trans,
    GstPadDirection direction, GstQuery * query);
static gboolean gst_smoothts_start (GstBaseTransform * trans);
static gboolean gst_smoothts_stop (GstBaseTransform * trans);
static GstFlowReturn gst_smoothts_transform_ip (GstBaseTransform * trans,
    GstBuffer * buf);


#define DEFAULT_TS_STEP              0
#define DEFAULT_ALLOWED_ERROR        0

enum
{
  PROP_0,
  PROP_TS_STEP,
  PROP_ALLOWED_ERROR
};

/* pad templates */

static GstStaticPadTemplate gst_smoothts_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS_ANY
    );

static GstStaticPadTemplate gst_smoothts_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS_ANY
    );


/* class initialization */

G_DEFINE_TYPE_WITH_CODE (GstSmoothts, gst_smoothts, GST_TYPE_BASE_TRANSFORM,
    GST_DEBUG_CATEGORY_INIT (gst_smoothts_debug_category, "smoothts", 0,
        "debug category for smoothts element"));

static void
gst_smoothts_class_init (GstSmoothtsClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstBaseTransformClass *base_transform_class =
      GST_BASE_TRANSFORM_CLASS (klass);

  /* Setting up pads and setting metadata should be moved to
     base_class_init if you intend to subclass this class. */
  gst_element_class_add_static_pad_template (GST_ELEMENT_CLASS (klass),
      &gst_smoothts_src_template);
  gst_element_class_add_static_pad_template (GST_ELEMENT_CLASS (klass),
      &gst_smoothts_sink_template);

  gst_element_class_set_static_metadata (GST_ELEMENT_CLASS (klass),
      "FIXME Long name", "Generic", "FIXME Description",
      "FIXME <fixme@example.com>");

  gobject_class->set_property = gst_smoothts_set_property;
  gobject_class->get_property = gst_smoothts_get_property;

  g_object_class_install_property (gobject_class, PROP_TS_STEP,
    g_param_spec_uint64 ("ts-step", "Timestamp Step",
          "Nanoseconds expected as delta between subsequent buffers", 0, G_MAXUINT64,
          DEFAULT_TS_STEP, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_ALLOWED_ERROR,
      g_param_spec_uint64 ("allowed-error", "Allowed Error", "Error allowed for smoothing",
          0, G_MAXUINT64, DEFAULT_ALLOWED_ERROR,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  gobject_class->dispose = gst_smoothts_dispose;
  gobject_class->finalize = gst_smoothts_finalize;
  base_transform_class->accept_caps =
      GST_DEBUG_FUNCPTR (gst_smoothts_accept_caps);
  base_transform_class->query = GST_DEBUG_FUNCPTR (gst_smoothts_query);
  base_transform_class->start = GST_DEBUG_FUNCPTR (gst_smoothts_start);
  base_transform_class->stop = GST_DEBUG_FUNCPTR (gst_smoothts_stop);
  base_transform_class->transform_ip =
      GST_DEBUG_FUNCPTR (gst_smoothts_transform_ip);

}

static void
gst_smoothts_init (GstSmoothts * smoothts)
{
    smoothts->ts_step = DEFAULT_TS_STEP;
    smoothts->allowed_error = DEFAULT_ALLOWED_ERROR;
    smoothts->last_ts = 0;
}

void
gst_smoothts_set_property (GObject * object, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  GstSmoothts *smoothts = GST_SMOOTHTS (object);

  GST_DEBUG_OBJECT (smoothts, "set_property");

  switch (property_id) {
    case PROP_TS_STEP:
      smoothts->ts_step = g_value_get_uint64 (value);
      break;
    case PROP_ALLOWED_ERROR:
      smoothts->allowed_error = g_value_get_uint64 (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

void
gst_smoothts_get_property (GObject * object, guint property_id,
    GValue * value, GParamSpec * pspec)
{
  GstSmoothts *smoothts = GST_SMOOTHTS (object);

  GST_DEBUG_OBJECT (smoothts, "get_property");

  switch (property_id) {
    case PROP_TS_STEP:
      g_value_set_uint (value, smoothts->ts_step);
      break;
    case PROP_ALLOWED_ERROR:
      g_value_set_int (value, smoothts->allowed_error);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

void
gst_smoothts_dispose (GObject * object)
{
  GstSmoothts *smoothts = GST_SMOOTHTS (object);

  GST_DEBUG_OBJECT (smoothts, "dispose");

  /* clean up as possible.  may be called multiple times */

  G_OBJECT_CLASS (gst_smoothts_parent_class)->dispose (object);
}

void
gst_smoothts_finalize (GObject * object)
{
  GstSmoothts *smoothts = GST_SMOOTHTS (object);

  GST_DEBUG_OBJECT (smoothts, "finalize");

  /* clean up object here */

  G_OBJECT_CLASS (gst_smoothts_parent_class)->finalize (object);
}

static gboolean
gst_smoothts_accept_caps (GstBaseTransform * trans, GstPadDirection direction,
    GstCaps * caps)
{
  gboolean ret;
  GstPad *pad;

  /* Proxy accept-caps */

  if (direction == GST_PAD_SRC)
    pad = GST_BASE_TRANSFORM_SINK_PAD (trans);
  else
    pad = GST_BASE_TRANSFORM_SRC_PAD (trans);

  ret = gst_pad_peer_query_accept_caps (pad, caps);

  return ret;
}

static gboolean
gst_smoothts_query (GstBaseTransform * trans, GstPadDirection direction,
    GstQuery * query)
{
  GstSmoothts *copyts = GST_SMOOTHTS (trans);
  gboolean ret;

  GST_DEBUG_OBJECT (copyts, "query");

  ret = GST_BASE_TRANSFORM_CLASS (gst_smoothts_parent_class)->query (trans, direction, query);

  return ret;
}

/* states */
static gboolean
gst_smoothts_start (GstBaseTransform * trans)
{
  GstSmoothts *smoothts = GST_SMOOTHTS (trans);

  GST_DEBUG_OBJECT (smoothts, "start");

  return TRUE;
}

static gboolean
gst_smoothts_stop (GstBaseTransform * trans)
{
  GstSmoothts *smoothts = GST_SMOOTHTS (trans);

  GST_DEBUG_OBJECT (smoothts, "stop");

  return TRUE;
}

static GstFlowReturn
gst_smoothts_transform_ip (GstBaseTransform * trans, GstBuffer * buf)
{
  GstSmoothts *smoothts = GST_SMOOTHTS (trans);

  GST_DEBUG_OBJECT (smoothts, "transform_ip");

  if (smoothts->ts_step != 0) {
    GstClockTime expected_ts = smoothts->last_ts + smoothts->ts_step;
    if (ABS((guint64)(expected_ts - buf->pts)) < smoothts->allowed_error) {
      buf->pts = expected_ts;
    }
  }

  smoothts->last_ts = buf->pts;

  return GST_FLOW_OK;
}

static gboolean
plugin_init (GstPlugin * plugin)
{

  /* FIXME Remember to set the rank if it's an element that is meant
     to be autoplugged by decodebin. */
  return gst_element_register (plugin, "smoothts", GST_RANK_NONE,
      GST_TYPE_SMOOTHTS);
}

/* FIXME: these are normally defined by the GStreamer build system.
   If you are creating an element to be included in gst-plugins-*,
   remove these, as they're always defined.  Otherwise, edit as
   appropriate for your external plugin package. */
#ifndef VERSION
#define VERSION "0.0.FIXME"
#endif
#ifndef PACKAGE
#define PACKAGE "FIXME_package"
#endif
#ifndef PACKAGE_NAME
#define PACKAGE_NAME "FIXME_package_name"
#endif
#ifndef GST_PACKAGE_ORIGIN
#define GST_PACKAGE_ORIGIN "http://FIXME.org/"
#endif

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    smoothts,
    "FIXME plugin description",
    plugin_init, VERSION, "LGPL", PACKAGE_NAME, GST_PACKAGE_ORIGIN)
