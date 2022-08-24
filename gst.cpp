
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <pthread.h>
#include <unistd.h>
#include <gst/gst.h>  // apt install -y libgstreamer1.0-dev
#include <gst/app/gstappsrc.h>  // apt install -y libgstreamer-plugins-base1.0-dev
#include <gst/app/gstappsink.h>
#include <iostream>
#include <chrono>
#include "nvbufsurface.h"
#include "nvdsmeta.h"
#include "gstnvdsmeta.h"

using namespace std;

#include "gst.h"

chrono::high_resolution_clock::time_point curr_time;

GstElement *pipeline_global;

void break_handler(int s)
{
  printf("Caught signal %d\n", s);
  gst_element_send_event (pipeline_global, gst_event_new_eos());
}

#define chk(a) if (!a) { g_printerr("Error line %d\r\n", __LINE__); return -1; }
#define xstr(a) str(a)
#define str(a) #a
#define new_element(var, type) printf("Creating %s\n", xstr(type)); GstElement *var = gst_element_factory_make(type, xstr(var)); chk(var);
#define link_element(elem1, elem2) printf("Linking %s and %s\n", xstr(elem1), xstr(elem2)); {auto ret = gst_element_link(elem1, elem2); chk(ret)};

GstPadProbeReturn probe_cb(GstPad *pad, GstPadProbeInfo *info, gpointer user_data)
{
  static int a = 0;
  // printf("Probe %d, type %X, id %ld, data %lX\n", a++, info->type, info->id, (uint64_t)info->data);

  GstBuffer *buf = (GstBuffer*)info->data;
  GstMapInfo map;

  if (gst_buffer_map(buf, &map, GST_MAP_READWRITE))
  {
    NvBufSurface *surface = (NvBufSurface*)map.data;
    NvBufSurfaceMap(surface, -1, -1, NVBUF_MAP_READ_WRITE);
    NvBufSurfaceSyncForCpu(surface, -1, -1);
    NvDsBatchMeta *batch_meta = gst_buffer_get_nvds_batch_meta(buf);

    // printf("data %lX, maxsize %ld, size %ld\n", (uint64_t)map.data, map.maxsize, map.size);
    // printf("width %d, height %d, addr %lX, pitch %d, dataSize %d, colorFormat %d, dataPtr %lX\n", width, height, addr, pitch, surface->surfaceList[0].dataSize, (uint64_t)surface->surfaceList[0].dataPtr);

    for (NvDsMetaList *l_frame = batch_meta->frame_meta_list; l_frame != NULL; l_frame = l_frame->next)
    {
      NvDsFrameMeta *frame_meta = (NvDsFrameMeta*)(l_frame->data);

      auto frame = frame_meta->frame_num;
      auto num_rects = frame_meta->num_obj_meta;

      auto width = surface->surfaceList[frame_meta->batch_id].width;
      auto height = surface->surfaceList[frame_meta->batch_id].height;
      auto addr = surface->surfaceList[frame_meta->batch_id].mappedAddr.addr[0];
      auto pitch = surface->surfaceList[frame_meta->batch_id].pitch;

      printf("frame %d, width %d, height %d, pitch %d, addr %lX, dataSize %d, colorFormat %d\n", frame, width, height, pitch, (uint64_t)addr, surface->surfaceList[0].dataSize, surface->surfaceList[0].colorFormat);
      // printf("frame %d, rects %d\n", frame, num_rects);

      for (NvDsMetaList *l_obj = frame_meta->obj_meta_list; l_obj != NULL; l_obj = l_obj->next)
      {
        NvDsObjectMeta *obj_meta = (NvDsObjectMeta*)(l_obj->data);

        int rleft = (int)obj_meta->rect_params.left;
        int rwidth = (int)obj_meta->rect_params.width;
        int rtop = (int)obj_meta->rect_params.top;
        int rheight = (int)obj_meta->rect_params.height;
        int class_id = obj_meta->class_id;

        printf("class_id %d, %s, [%d, %d, %d, %d], confidence %f\n", class_id, obj_meta->obj_label, rleft, rtop, rleft + rwidth - 1, rtop + rheight - 1, obj_meta->confidence);

        if (class_id == 1)  // Bikes - green
        {
          uint8_t *p = (uint8_t*)addr;

          for (int row = rtop; row < (rtop + rheight); row++)
          for (int col = rleft; col < (rleft + rwidth); col++)
          {
            int i = row * pitch + col * 4;
            p[i + 0] >>= 1;
            p[i + 2] >>= 1;
          }
        }

        if (class_id == 2)  // Persons - blue color
        {
          uint8_t *p = (uint8_t*)addr;

          for (int row = rtop; row < (rtop + rheight); row++)
          for (int col = rleft; col < (rleft + rwidth); col++)
          {
            int i = row * pitch + col * 4;
            p[i + 0] >>= 1;
            p[i + 1] >>= 1;
          }
        }

        if (class_id == 0)  // Cars - red color
        {
          uint8_t *p = (uint8_t*)addr;

          for (int row = rtop; row < (rtop + rheight); row++)
          for (int col = rleft; col < (rleft + rwidth); col++)
          {
            int i = row * pitch + col * 4;
            p[i + 1] >>= 1;
            p[i + 2] >>= 1;
          }
        }

        // kernel_wrap((uint8_t*)surface->surfaceList[0].dataPtr, width, height, rleft, rtop, rwidth, rheight);
      }
    }
  }

  gst_buffer_unmap(buf, &map);

  return GST_PAD_PROBE_OK;
}

int gst_my()
{
  // Init GStreamer, create Pipeline
  gst_init(NULL, NULL);
  auto *pipeline = gst_pipeline_new("pipeline"); chk(pipeline);
  pipeline_global = pipeline;

  // Create Elements
  new_element(source, "filesrc");
  new_element(parser, "h264parse");
  new_element(decoder, "nvv4l2decoder");
  new_element(streammux, "nvstreammux");
  new_element(pgie, "nvinfer");
  new_element(nvconv, "nvvideoconvert");
  new_element(overlay, "nvoverlaysink");

  // Set Element properties
  printf("Setting Element properties\n");
  g_object_set(source, "location", "video.h264", NULL);
  g_object_set(streammux, "width", 1920, "height", 1080, "batch-size", 1, "batched-push-timeout", 4000000, NULL);
  g_object_set(pgie, "config-file-path", "pgie_config.txt", NULL);

  // Add Elements to Pipeline
  printf("Adding Elements to Pipeline\n");
  gst_bin_add_many(GST_BIN(pipeline), source, parser, decoder, streammux, pgie, nvconv, overlay, NULL);

  // Link Elements
  link_element(source, parser);
  link_element(parser, decoder);

  printf("Linking decoder and streammux\n");
  auto decoder_pad = gst_element_get_static_pad(decoder, "src"); chk(decoder_pad);
  auto streammux_pad = gst_element_get_request_pad(streammux, "sink_0"); chk(streammux_pad);
  {auto rc = !gst_pad_link(decoder_pad, streammux_pad); chk(rc);}
  gst_object_unref(decoder_pad);
  gst_object_unref(streammux_pad);

  link_element(streammux, pgie);
  link_element(pgie, nvconv);

  printf("Linking nvconv and overlay\n");
  auto *caps = gst_caps_from_string("video/x-raw(memory:NVMM), format=RGBA");
  gst_element_link_filtered(nvconv, overlay, caps);
  gst_caps_unref(caps);

  // Add Message Handler
  printf("Adding message handler\n");
  auto *bus = gst_element_get_bus(pipeline); chk(bus);

  // Create Loop
  printf("Adding Main Loop\n");
  auto *loop = g_main_loop_new(NULL, FALSE); chk(loop);

  // Set Buffer Probe
  gst_pad_add_probe(gst_element_get_static_pad(nvconv, "src"), GST_PAD_PROBE_TYPE_BUFFER, probe_cb, NULL, NULL);

  // Run Pipeline
  {auto rc = gst_element_set_state(pipeline, GST_STATE_PLAYING); chk(rc)};

  GstMessage *msg;
  msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, (GstMessageType)(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));

  GError *err;
  gchar *debug_info;

  switch(GST_MESSAGE_TYPE(msg))
  {
    case GST_MESSAGE_ERROR:
      gst_message_parse_error(msg, &err, &debug_info);
      g_printerr("Error received from element %s: %s\n", GST_OBJECT_NAME(msg->src), err->message);
      g_printerr("Debugging information: %s\n", debug_info ? debug_info : "none");
      g_clear_error(&err);
      g_free(debug_info);
      break;

    case GST_MESSAGE_EOS:
      g_print("EOS reached.\n");
      break;

    default:
      g_printerr("Unexpected message received.\n");
      break;
  }

  gst_message_unref(msg);
  gst_object_unref(bus);
  gst_element_set_state(pipeline, GST_STATE_NULL);
  gst_object_unref(pipeline);

  g_printerr("Done\n\r");

  return 0;
}
