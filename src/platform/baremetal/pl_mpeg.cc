#include "../../../lib/pl_mpeg/pl_mpeg.h"
//#include <stdlib.h>
#include "/usr/share/etl/etl-20.39.4/include/etl/absolute.h"
#include <string.h>

plm_t *plm_create_with_filename(const char *filename, plm_t *self_ptr) {
  plm_buffer_t *buffer = plm_buffer_create_with_filename(filename);
  if (!buffer) {
    return NULL;
  }

  plm_create_with_buffer(buffer, TRUE, self_ptr);
  return self_ptr;
}

plm_t *plm_create_with_file(FILE *fh, int close_when_done, plm_t *self_ptr) {
  plm_buffer_t *buffer = plm_buffer_create_with_file(fh, close_when_done);

  plm_create_with_buffer(buffer, TRUE, self_ptr);
  return self_ptr;
}

plm_t *plm_create_with_memory(uint8_t *bytes, size_t length, int free_when_done,
                              plm_t *self_ptr) {
  plm_buffer_t *buffer =
      plm_buffer_create_with_memory(bytes, length, free_when_done);

  plm_create_with_buffer(buffer, TRUE, self_ptr);
  return self_ptr;
}

void plm_create_with_buffer(plm_buffer_t *buffer, int destroy_when_done,
                            plm_t *self) {
  self->demux = plm_demux_create(buffer, destroy_when_done);
  self->video_enabled = TRUE;
  self->audio_enabled = TRUE;
  plm_init_decoders(self);
}

int plm_init_decoders(plm_t *self) {
  if (self->has_decoders) {
    return TRUE;
  }

  if (!plm_demux_has_headers(self->demux)) {
    return FALSE;
  }

  if (plm_demux_get_num_video_streams(self->demux) > 0) {
    if (self->video_enabled) {
      self->video_packet_type = PLM_DEMUX_PACKET_VIDEO_1;
    }
    if (!self->video_decoder) {
      self->video_buffer =
          plm_buffer_create_with_capacity(PLM_BUFFER_DEFAULT_SIZE);
      plm_buffer_set_load_callback(self->video_buffer, plm_read_video_packet,
                                   self);
      self->video_decoder =
          plm_video_create_with_buffer(self->video_buffer, TRUE);
    }
  }

  if (plm_demux_get_num_audio_streams(self->demux) > 0) {
    if (self->audio_enabled) {
      self->audio_packet_type =
          PLM_DEMUX_PACKET_AUDIO_1 + self->audio_stream_index;
    }
    if (!self->audio_decoder) {
      self->audio_buffer =
          plm_buffer_create_with_capacity(PLM_BUFFER_DEFAULT_SIZE);
      plm_buffer_set_load_callback(self->audio_buffer, plm_read_audio_packet,
                                   self);
      self->audio_decoder =
          plm_audio_create_with_buffer(self->audio_buffer, TRUE);
    }
  }

  self->has_decoders = TRUE;
  return TRUE;
}

void plm_destroy(plm_t *self) {
  if (self->video_decoder) {
    plm_video_destroy(self->video_decoder);
  }
  if (self->audio_decoder) {
    plm_audio_destroy(self->audio_decoder);
  }

  plm_demux_destroy(self->demux);
}

int plm_get_audio_enabled(plm_t *self) { return self->audio_enabled; }

int plm_has_headers(plm_t *self) {
  if (!plm_demux_has_headers(self->demux)) {
    return FALSE;
  }

  if (!plm_init_decoders(self)) {
    return FALSE;
  }

  if ((self->video_decoder && !plm_video_has_header(self->video_decoder)) ||
      (self->audio_decoder && !plm_audio_has_header(self->audio_decoder))) {
    return FALSE;
  }

  return TRUE;
}

int plm_probe(plm_t *self, size_t probesize) {
  int found_streams = plm_demux_probe(self->demux, probesize);
  if (!found_streams) {
    return FALSE;
  }

  // Re-init decoders
  self->has_decoders = FALSE;
  self->video_packet_type = 0;
  self->audio_packet_type = 0;
  return plm_init_decoders(self);
}

void plm_set_audio_enabled(plm_t *self, int enabled) {
  self->audio_enabled = enabled;

  if (!enabled) {
    self->audio_packet_type = 0;
    return;
  }

  self->audio_packet_type =
      (plm_init_decoders(self) && self->audio_decoder)
          ? PLM_DEMUX_PACKET_AUDIO_1 + self->audio_stream_index
          : 0;
}

void plm_set_audio_stream(plm_t *self, int stream_index) {
  if (stream_index < 0 || stream_index > 3) {
    return;
  }
  self->audio_stream_index = stream_index;

  // Set the correct audio_packet_type
  plm_set_audio_enabled(self, self->audio_enabled);
}

int plm_get_video_enabled(plm_t *self) { return self->video_enabled; }

void plm_set_video_enabled(plm_t *self, int enabled) {
  self->video_enabled = enabled;

  if (!enabled) {
    self->video_packet_type = 0;
    return;
  }

  self->video_packet_type = (plm_init_decoders(self) && self->video_decoder)
                                ? PLM_DEMUX_PACKET_VIDEO_1
                                : 0;
}

int plm_get_num_video_streams(plm_t *self) {
  return plm_demux_get_num_video_streams(self->demux);
}

int plm_get_width(plm_t *self) {
  return (plm_init_decoders(self) && self->video_decoder)
             ? plm_video_get_width(self->video_decoder)
             : 0;
}

int plm_get_height(plm_t *self) {
  return (plm_init_decoders(self) && self->video_decoder)
             ? plm_video_get_height(self->video_decoder)
             : 0;
}

double plm_get_framerate(plm_t *self) {
  return (plm_init_decoders(self) && self->video_decoder)
             ? plm_video_get_framerate(self->video_decoder)
             : 0;
}

double plm_get_pixel_aspect_ratio(plm_t *self) {
  return (plm_init_decoders(self) && self->video_decoder)
             ? plm_video_get_pixel_aspect_ratio(self->video_decoder)
             : 0;
}

int plm_get_num_audio_streams(plm_t *self) {
  return plm_demux_get_num_audio_streams(self->demux);
}

int plm_get_samplerate(plm_t *self) {
  return (plm_init_decoders(self) && self->audio_decoder)
             ? plm_audio_get_samplerate(self->audio_decoder)
             : 0;
}

double plm_get_audio_lead_time(plm_t *self) { return self->audio_lead_time; }

void plm_set_audio_lead_time(plm_t *self, double lead_time) {
  self->audio_lead_time = lead_time;
}

double plm_get_time(plm_t *self) { return self->time; }

double plm_get_duration(plm_t *self) {
  return plm_demux_get_duration(self->demux, PLM_DEMUX_PACKET_VIDEO_1);
}

void plm_rewind(plm_t *self) {
  if (self->video_decoder) {
    plm_video_rewind(self->video_decoder);
  }

  if (self->audio_decoder) {
    plm_audio_rewind(self->audio_decoder);
  }

  plm_demux_rewind(self->demux);
  self->time = 0;
}

int plm_get_loop(plm_t *self) { return self->loop; }

void plm_set_loop(plm_t *self, int loop) { self->loop = loop; }

int plm_has_ended(plm_t *self) { return self->has_ended; }

void plm_set_video_decode_callback(plm_t *self, plm_video_decode_callback fp,
                                   void *user) {
  self->video_decode_callback = fp;
  self->video_decode_callback_user_data = user;
}

void plm_set_audio_decode_callback(plm_t *self, plm_audio_decode_callback fp,
                                   void *user) {
  self->audio_decode_callback = fp;
  self->audio_decode_callback_user_data = user;
}

void plm_decode(plm_t *self, double tick) {
  if (!plm_init_decoders(self)) {
    return;
  }

  int decode_video = (self->video_decode_callback && self->video_packet_type);
  int decode_audio = (self->audio_decode_callback && self->audio_packet_type);

  if (!decode_video && !decode_audio) {
    // Nothing to do here
    return;
  }

  int did_decode = FALSE;
  int decode_video_failed = FALSE;
  int decode_audio_failed = FALSE;

  double video_target_time = self->time + tick;
  double audio_target_time = self->time + tick + self->audio_lead_time;

  do {
    did_decode = FALSE;

    if (decode_video &&
        plm_video_get_time(self->video_decoder) < video_target_time) {
      plm_frame_t *frame = plm_video_decode(self->video_decoder);
      if (frame) {
        self->video_decode_callback(self, frame,
                                    self->video_decode_callback_user_data);
        did_decode = TRUE;
      } else {
        decode_video_failed = TRUE;
      }
    }

    if (decode_audio &&
        plm_audio_get_time(self->audio_decoder) < audio_target_time) {
      plm_samples_t *samples = plm_audio_decode(self->audio_decoder);
      if (samples) {
        self->audio_decode_callback(self, samples,
                                    self->audio_decode_callback_user_data);
        did_decode = TRUE;
      } else {
        decode_audio_failed = TRUE;
      }
    }
  } while (did_decode);

  // Did all sources we wanted to decode fail and the demuxer is at the end?
  if ((!decode_video || decode_video_failed) &&
      (!decode_audio || decode_audio_failed) &&
      plm_demux_has_ended(self->demux)) {
    plm_handle_end(self);
    return;
  }

  self->time += tick;
}

plm_frame_t *plm_decode_video(plm_t *self) {
  if (!plm_init_decoders(self)) {
    return NULL;
  }

  if (!self->video_packet_type) {
    return NULL;
  }

  plm_frame_t *frame = plm_video_decode(self->video_decoder);
  if (frame) {
    self->time = frame->time;
  } else if (plm_demux_has_ended(self->demux)) {
    plm_handle_end(self);
  }
  return frame;
}

plm_samples_t *plm_decode_audio(plm_t *self) {
  if (!plm_init_decoders(self)) {
    return NULL;
  }

  if (!self->audio_packet_type) {
    return NULL;
  }

  plm_samples_t *samples = plm_audio_decode(self->audio_decoder);
  if (samples) {
    self->time = samples->time;
  } else if (plm_demux_has_ended(self->demux)) {
    plm_handle_end(self);
  }
  return samples;
}

void plm_handle_end(plm_t *self) {
  if (self->loop) {
    plm_rewind(self);
  } else {
    self->has_ended = TRUE;
  }
}

void plm_read_video_packet(plm_buffer_t *buffer, void *user) {
  PLM_UNUSED(buffer);
  plm_t *self = (plm_t *)user;
  plm_read_packets(self, self->video_packet_type);
}

void plm_read_audio_packet(plm_buffer_t *buffer, void *user) {
  PLM_UNUSED(buffer);
  plm_t *self = (plm_t *)user;
  plm_read_packets(self, self->audio_packet_type);
}

void plm_read_packets(plm_t *self, int requested_type) {
  plm_packet_t *packet;
  while ((packet = plm_demux_decode(self->demux))) {
    if (packet->type == self->video_packet_type) {
      plm_buffer_write(self->video_buffer, packet->data, packet->length);
    } else if (packet->type == self->audio_packet_type) {
      plm_buffer_write(self->audio_buffer, packet->data, packet->length);
    }

    if (packet->type == requested_type) {
      return;
    }
  }

  if (plm_demux_has_ended(self->demux)) {
    if (self->video_buffer) {
      plm_buffer_signal_end(self->video_buffer);
    }
    if (self->audio_buffer) {
      plm_buffer_signal_end(self->audio_buffer);
    }
  }
}

plm_frame_t *plm_seek_frame(plm_t *self, double time, int seek_exact) {
  if (!plm_init_decoders(self)) {
    return NULL;
  }

  if (!self->video_packet_type) {
    return NULL;
  }

  int type = self->video_packet_type;

  double start_time = plm_demux_get_start_time(self->demux, type);
  double duration = plm_demux_get_duration(self->demux, type);

  if (time < 0) {
    time = 0;
  } else if (time > duration) {
    time = duration;
  }

  plm_packet_t *packet = plm_demux_seek(self->demux, time, type, TRUE);
  if (!packet) {
    return NULL;
  }

  // Disable writing to the audio buffer while decoding video
  int previous_audio_packet_type = self->audio_packet_type;
  self->audio_packet_type = 0;

  // Clear video buffer and decode the found packet
  plm_video_rewind(self->video_decoder);
  plm_video_set_time(self->video_decoder, packet->pts - start_time);
  plm_buffer_write(self->video_buffer, packet->data, packet->length);
  plm_frame_t *frame = plm_video_decode(self->video_decoder);

  // If we want to seek to an exact frame, we have to decode all frames
  // on top of the intra frame we just jumped to.
  if (seek_exact) {
    while (frame && frame->time < time) {
      frame = plm_video_decode(self->video_decoder);
    }
  }

  // Enable writing to the audio buffer again?
  self->audio_packet_type = previous_audio_packet_type;

  if (frame) {
    self->time = frame->time;
  }

  self->has_ended = FALSE;
  return frame;
}

int plm_seek(plm_t *self, double time, int seek_exact) {
  plm_frame_t *frame = plm_seek_frame(self, time, seek_exact);

  if (!frame) {
    return FALSE;
  }

  if (self->video_decode_callback) {
    self->video_decode_callback(self, frame,
                                self->video_decode_callback_user_data);
  }

  // If audio is not enabled we are done here.
  if (!self->audio_packet_type) {
    return TRUE;
  }

  // Sync up Audio. This demuxes more packets until the first audio packet
  // with a PTS greater than the current time is found. plm_decode() is then
  // called to decode enough audio data to satisfy the audio_lead_time.

  double start_time =
      plm_demux_get_start_time(self->demux, self->video_packet_type);
  plm_audio_rewind(self->audio_decoder);

  plm_packet_t *packet = NULL;
  while ((packet = plm_demux_decode(self->demux))) {
    if (packet->type == self->video_packet_type) {
      plm_buffer_write(self->video_buffer, packet->data, packet->length);
    } else if (packet->type == self->audio_packet_type &&
               packet->pts - start_time > self->time) {
      plm_audio_set_time(self->audio_decoder, packet->pts - start_time);
      plm_buffer_write(self->audio_buffer, packet->data, packet->length);
      plm_decode(self, 0);
      break;
    }
  }

  return TRUE;
}


plm_buffer_t *plm_buffer_create_with_filename(const char *filename) {
  return NULL; 
  /*FILE *fh = fopen(filename, "rb");
  if (!fh) {
    return NULL;
  }
  return plm_buffer_create_with_file(fh, TRUE);*/
}

plm_buffer_t *plm_buffer_create_with_file(FILE *fh, int close_when_done) {
  return NULL;
  /*
  plm_buffer_t *self = plm_buffer_create_with_capacity(PLM_BUFFER_DEFAULT_SIZE);
  self->fh = fh;
  self->close_when_done = close_when_done;
  self->mode = PLM_BUFFER_MODE_FILE;
  self->discard_read_bytes = TRUE;

  fseek(self->fh, 0, SEEK_END);
  self->total_size = ftell(self->fh);
  fseek(self->fh, 0, SEEK_SET);

  plm_buffer_set_load_callback(self, plm_buffer_load_file_callback, NULL);
  return self;*/
}

plm_buffer_t *plm_buffer_create_with_memory(uint8_t *bytes, size_t length,
                                            int free_when_done) {
  plm_buffer_t *self = &static_buffer_w_memory_holder;
  memset(self, 0, sizeof(plm_buffer_t));
  self->capacity = length;
  self->length = length;
  self->total_size = length;
  self->free_when_done = free_when_done;
  memcpy(self->bytes, bytes, PLM_BUFFER_DEFAULT_SIZE);
  self->mode = PLM_BUFFER_MODE_FIXED_MEM;
  self->discard_read_bytes = FALSE;
  return self;
}

plm_buffer_t *plm_buffer_create_with_capacity(size_t capacity) {
  plm_buffer_t *self = &static_buffer_holder[buffer_n];
  buffer_n++;
  memset(self, 0, sizeof(plm_buffer_t));
  self->capacity = capacity;
  self->free_when_done = TRUE;
  self->mode = PLM_BUFFER_MODE_RING;
  self->discard_read_bytes = TRUE;
  return self;
}

plm_buffer_t *plm_buffer_create_for_appending(size_t initial_capacity) {
  plm_buffer_t *self = plm_buffer_create_with_capacity(initial_capacity);
  self->mode = PLM_BUFFER_MODE_APPEND;
  self->discard_read_bytes = FALSE;
  return self;
}

void plm_buffer_destroy(plm_buffer_t *self) {
  if (self->fh && self->close_when_done) {
    //fclose(self->fh);
  }
}

size_t plm_buffer_get_size(plm_buffer_t *self) {
  return (self->mode == PLM_BUFFER_MODE_FILE) ? self->total_size : self->length;
}

size_t plm_buffer_get_remaining(plm_buffer_t *self) {
  return self->length - (self->bit_index >> 3);
}

size_t plm_buffer_write(plm_buffer_t *self, uint8_t *bytes, size_t length) {
  if (self->mode == PLM_BUFFER_MODE_FIXED_MEM) {
    return 0;
  }

  if (self->discard_read_bytes) {
    // This should be a ring buffer, but instead it just shifts all unread
    // data to the beginning of the buffer and appends new data at the end.
    // Seems to be good enough.

    plm_buffer_discard_read_bytes(self);
    if (self->mode == PLM_BUFFER_MODE_RING) {
      self->total_size = 0;
    }
  }

  memcpy(self->bytes + self->length, bytes, length);
  self->length += length;
  self->has_ended = FALSE;
  return length;
}

void plm_buffer_signal_end(plm_buffer_t *self) {
  self->total_size = self->length;
}

void plm_buffer_set_load_callback(plm_buffer_t *self,
                                  plm_buffer_load_callback fp, void *user) {
  self->load_callback = fp;
  self->load_callback_user_data = user;
}

void plm_buffer_rewind(plm_buffer_t *self) { plm_buffer_seek(self, 0); }

void plm_buffer_seek(plm_buffer_t *self, size_t pos) {
  self->has_ended = FALSE;

  if (self->mode == PLM_BUFFER_MODE_FILE) {
    fseek(self->fh, pos, SEEK_SET);
    self->bit_index = 0;
    self->length = 0;
  } else if (self->mode == PLM_BUFFER_MODE_RING) {
    if (pos != 0) {
      // Seeking to non-0 is forbidden for dynamic-mem buffers
      return;
    }
    self->bit_index = 0;
    self->length = 0;
    self->total_size = 0;
  } else if (pos < self->length) {
    self->bit_index = pos << 3;
  }
}

size_t plm_buffer_tell(plm_buffer_t *self) {
  return self->mode == PLM_BUFFER_MODE_FILE
             ? ftell(self->fh) + (self->bit_index >> 3) - self->length
             : self->bit_index >> 3;
}

void plm_buffer_discard_read_bytes(plm_buffer_t *self) {
  size_t byte_pos = self->bit_index >> 3;
  if (byte_pos == self->length) {
    self->bit_index = 0;
    self->length = 0;
  } else if (byte_pos > 0) {
    memmove(self->bytes, self->bytes + byte_pos, self->length - byte_pos);
    self->bit_index -= byte_pos << 3;
    self->length -= byte_pos;
  }
}

void plm_buffer_load_file_callback(plm_buffer_t *self, void *user) {
  PLM_UNUSED(user);

  if (self->discard_read_bytes) {
    plm_buffer_discard_read_bytes(self);
  }

  size_t bytes_available = self->capacity - self->length;
  size_t bytes_read =
      fread(self->bytes + self->length, 1, bytes_available, self->fh);
  self->length += bytes_read;

  if (bytes_read == 0) {
    self->has_ended = TRUE;
  }
}

int plm_buffer_has_ended(plm_buffer_t *self) { return self->has_ended; }

int plm_buffer_has(plm_buffer_t *self, size_t count) {
  if (((self->length << 3) - self->bit_index) >= count) {
    return TRUE;
  }

  if (self->load_callback) {
    self->load_callback(self, self->load_callback_user_data);

    if (((self->length << 3) - self->bit_index) >= count) {
      return TRUE;
    }
  }

  if (self->total_size != 0 && self->length == self->total_size) {
    self->has_ended = TRUE;
  }
  return FALSE;
}

int plm_buffer_read(plm_buffer_t *self, int count) {
  if (!plm_buffer_has(self, count)) {
    return 0;
  }

  int value = 0;
  while (count) {
    int current_byte = self->bytes[self->bit_index >> 3];

    int remaining = 8 - (self->bit_index & 7);         // Remaining bits in byte
    int read = remaining < count ? remaining : count;  // Bits in self run
    int shift = remaining - read;
    int mask = (0xff >> (8 - read));

    value = (value << read) | ((current_byte & (mask << shift)) >> shift);

    self->bit_index += read;
    count -= read;
  }

  return value;
}

void plm_buffer_align(plm_buffer_t *self) {
  self->bit_index = ((self->bit_index + 7) >> 3) << 3;  // Align to next byte
}

void plm_buffer_skip(plm_buffer_t *self, size_t count) {
  if (plm_buffer_has(self, count)) {
    self->bit_index += count;
  }
}

int plm_buffer_skip_bytes(plm_buffer_t *self, uint8_t v) {
  plm_buffer_align(self);
  int skipped = 0;
  while (plm_buffer_has(self, 8) && self->bytes[self->bit_index >> 3] == v) {
    self->bit_index += 8;
    skipped++;
  }
  return skipped;
}

int plm_buffer_next_start_code(plm_buffer_t *self) {
  plm_buffer_align(self);

  while (plm_buffer_has(self, (5 << 3))) {
    size_t byte_index = (self->bit_index) >> 3;
    if (self->bytes[byte_index] == 0x00 &&
        self->bytes[byte_index + 1] == 0x00 &&
        self->bytes[byte_index + 2] == 0x01) {
      self->bit_index = (byte_index + 4) << 3;
      return self->bytes[byte_index + 3];
    }
    self->bit_index += 8;
  }
  return -1;
}

int plm_buffer_find_start_code(plm_buffer_t *self, int code) {
  int current = 0;
  while (TRUE) {
    current = plm_buffer_next_start_code(self);
    if (current == code || current == -1) {
      return current;
    }
  }
  return -1;
}

int plm_buffer_has_start_code(plm_buffer_t *self, int code) {
  size_t previous_bit_index = self->bit_index;
  int previous_discard_read_bytes = self->discard_read_bytes;

  self->discard_read_bytes = FALSE;
  int current = plm_buffer_find_start_code(self, code);

  self->bit_index = previous_bit_index;
  self->discard_read_bytes = previous_discard_read_bytes;
  return current;
}

int plm_buffer_peek_non_zero(plm_buffer_t *self, int bit_count) {
  if (!plm_buffer_has(self, bit_count)) {
    return FALSE;
  }

  int val = plm_buffer_read(self, bit_count);
  self->bit_index -= bit_count;
  return val != 0;
}

int16_t plm_buffer_read_vlc(plm_buffer_t *self, const plm_vlc_t *table) {
  plm_vlc_t state = {0, 0};
  do {
    state = table[state.index + plm_buffer_read(self, 1)];
  } while (state.index > 0);
  return state.value;
}

uint16_t plm_buffer_read_vlc_uint(plm_buffer_t *self,
                                  const plm_vlc_uint_t *table) {
  return (uint16_t)plm_buffer_read_vlc(self, (const plm_vlc_t *)table);
}


plm_demux_t *plm_demux_create(plm_buffer_t *buffer, int destroy_when_done) {
  plm_demux_t *self = &static_demux_holder;

  self->buffer = buffer;
  self->destroy_buffer_when_done = destroy_when_done;

  self->start_time = PLM_PACKET_INVALID_TS;
  self->duration = PLM_PACKET_INVALID_TS;
  self->start_code = -1;

  plm_demux_has_headers(self);
  return self;
}

void plm_demux_destroy(plm_demux_t *self) {
  if (self->destroy_buffer_when_done) {
    plm_buffer_destroy(self->buffer);
  }
}

int plm_demux_has_headers(plm_demux_t *self) {
  if (self->has_headers) {
    return TRUE;
  }

  // Decode pack header
  if (!self->has_pack_header) {
    if (self->start_code != PLM_START_PACK &&
        plm_buffer_find_start_code(self->buffer, PLM_START_PACK) == -1) {
      return FALSE;
    }

    self->start_code = PLM_START_PACK;
    if (!plm_buffer_has(self->buffer, 64)) {
      return FALSE;
    }
    self->start_code = -1;

    if (plm_buffer_read(self->buffer, 4) != 0x02) {
      return FALSE;
    }

    self->system_clock_ref = plm_demux_decode_time(self);
    plm_buffer_skip(self->buffer, 1);
    plm_buffer_skip(self->buffer, 22);  // mux_rate * 50
    plm_buffer_skip(self->buffer, 1);

    self->has_pack_header = TRUE;
  }

  // Decode system header
  if (!self->has_system_header) {
    if (self->start_code != PLM_START_SYSTEM &&
        plm_buffer_find_start_code(self->buffer, PLM_START_SYSTEM) == -1) {
      return FALSE;
    }

    self->start_code = PLM_START_SYSTEM;
    if (!plm_buffer_has(self->buffer, 56)) {
      return FALSE;
    }
    self->start_code = -1;

    plm_buffer_skip(self->buffer, 16);  // header_length
    plm_buffer_skip(self->buffer, 24);  // rate bound
    self->num_audio_streams = plm_buffer_read(self->buffer, 6);
    plm_buffer_skip(self->buffer, 5);  // misc flags
    self->num_video_streams = plm_buffer_read(self->buffer, 5);

    self->has_system_header = TRUE;
  }

  self->has_headers = TRUE;
  return TRUE;
}

int plm_demux_probe(plm_demux_t *self, size_t probesize) {
  int previous_pos = plm_buffer_tell(self->buffer);

  int video_stream = FALSE;
  int audio_streams[4] = {FALSE, FALSE, FALSE, FALSE};
  do {
    self->start_code = plm_buffer_next_start_code(self->buffer);
    if (self->start_code == PLM_DEMUX_PACKET_VIDEO_1) {
      video_stream = TRUE;
    } else if (self->start_code >= PLM_DEMUX_PACKET_AUDIO_1 &&
               self->start_code <= PLM_DEMUX_PACKET_AUDIO_4) {
      audio_streams[self->start_code - PLM_DEMUX_PACKET_AUDIO_1] = TRUE;
    }
  } while (self->start_code != -1 &&
           plm_buffer_tell(self->buffer) - previous_pos < probesize);

  self->num_video_streams = video_stream ? 1 : 0;
  self->num_audio_streams = 0;
  for (int i = 0; i < 4; i++) {
    if (audio_streams[i]) {
      self->num_audio_streams++;
    }
  }

  plm_demux_buffer_seek(self, previous_pos);
  return (self->num_video_streams || self->num_audio_streams);
}

int plm_demux_get_num_video_streams(plm_demux_t *self) {
  return plm_demux_has_headers(self) ? self->num_video_streams : 0;
}

int plm_demux_get_num_audio_streams(plm_demux_t *self) {
  return plm_demux_has_headers(self) ? self->num_audio_streams : 0;
}

void plm_demux_rewind(plm_demux_t *self) {
  plm_buffer_rewind(self->buffer);
  self->current_packet.length = 0;
  self->next_packet.length = 0;
  self->start_code = -1;
}

int plm_demux_has_ended(plm_demux_t *self) {
  return plm_buffer_has_ended(self->buffer);
}

void plm_demux_buffer_seek(plm_demux_t *self, size_t pos) {
  plm_buffer_seek(self->buffer, pos);
  self->current_packet.length = 0;
  self->next_packet.length = 0;
  self->start_code = -1;
}

double plm_demux_get_start_time(plm_demux_t *self, int type) {
  if (self->start_time != PLM_PACKET_INVALID_TS) {
    return self->start_time;
  }

  int previous_pos = plm_buffer_tell(self->buffer);
  int previous_start_code = self->start_code;

  // Find first video PTS
  plm_demux_rewind(self);
  do {
    plm_packet_t *packet = plm_demux_decode(self);
    if (!packet) {
      break;
    }
    if (packet->type == type) {
      self->start_time = packet->pts;
    }
  } while (self->start_time == PLM_PACKET_INVALID_TS);

  plm_demux_buffer_seek(self, previous_pos);
  self->start_code = previous_start_code;
  return self->start_time;
}

double plm_demux_get_duration(plm_demux_t *self, int type) {
  size_t file_size = plm_buffer_get_size(self->buffer);

  if (self->duration != PLM_PACKET_INVALID_TS &&
      self->last_file_size == file_size) {
    return self->duration;
  }

  size_t previous_pos = plm_buffer_tell(self->buffer);
  int previous_start_code = self->start_code;

  // Find last video PTS. Start searching 64kb from the end and go further
  // back if needed.
  long start_range = 64 * 1024;
  long max_range = 4096 * 1024;
  for (long range = start_range; range <= max_range; range *= 2) {
    long seek_pos = file_size - range;
    if (seek_pos < 0) {
      seek_pos = 0;
      range = max_range;  // Make sure to bail after this round
    }
    plm_demux_buffer_seek(self, seek_pos);
    self->current_packet.length = 0;

    double last_pts = PLM_PACKET_INVALID_TS;
    plm_packet_t *packet = NULL;
    while ((packet = plm_demux_decode(self))) {
      if (packet->pts != PLM_PACKET_INVALID_TS && packet->type == type) {
        last_pts = packet->pts;
      }
    }
    if (last_pts != PLM_PACKET_INVALID_TS) {
      self->duration = last_pts - plm_demux_get_start_time(self, type);
      break;
    }
  }

  plm_demux_buffer_seek(self, previous_pos);
  self->start_code = previous_start_code;
  self->last_file_size = file_size;
  return self->duration;
}

plm_packet_t *plm_demux_seek(plm_demux_t *self, double seek_time, int type,
                             int force_intra) {
  if (!plm_demux_has_headers(self)) {
    return NULL;
  }

  // Using the current time, current byte position and the average bytes per
  // second for this file, try to jump to a byte position that hopefully has
  // packets containing timestamps within one second before to the desired
  // seek_time.

  // If we hit close to the seek_time scan through all packets to find the
  // last one (just before the seek_time) containing an intra frame.
  // Otherwise we should at least be closer than before. Calculate the bytes
  // per second for the jumped range and jump again.

  // The number of retries here is hard-limited to a generous amount. Usually
  // the correct range is found after 1--5 jumps, even for files with very
  // variable bitrates. If significantly more jumps are needed, there's
  // probably something wrong with the file and we just avoid getting into an
  // infinite loop. 32 retries should be enough for anybody.

  double duration = plm_demux_get_duration(self, type);
  long file_size = plm_buffer_get_size(self->buffer);
  long byterate = file_size / duration;

  double cur_time = self->last_decoded_pts;
  double scan_span = 1;

  if (seek_time > duration) {
    seek_time = duration;
  } else if (seek_time < 0) {
    seek_time = 0;
  }
  seek_time += self->start_time;

  for (int retry = 0; retry < 32; retry++) {
    int found_packet_with_pts = FALSE;
    int found_packet_in_range = FALSE;
    long last_valid_packet_start = -1;
    double first_packet_time = PLM_PACKET_INVALID_TS;

    long cur_pos = plm_buffer_tell(self->buffer);

    // Estimate byte offset and jump to it.
    long offset = (seek_time - cur_time - scan_span) * byterate;
    long seek_pos = cur_pos + offset;
    if (seek_pos < 0) {
      seek_pos = 0;
    } else if (seek_pos > file_size - 256) {
      seek_pos = file_size - 256;
    }

    plm_demux_buffer_seek(self, seek_pos);

    // Scan through all packets up to the seek_time to find the last packet
    // containing an intra frame.
    while (plm_buffer_find_start_code(self->buffer, type) != -1) {
      long packet_start = plm_buffer_tell(self->buffer);
      plm_packet_t *packet = plm_demux_decode_packet(self, type);

      // Skip packet if it has no PTS
      if (!packet || packet->pts == PLM_PACKET_INVALID_TS) {
        continue;
      }

      // Bail scanning through packets if we hit one that is outside
      // seek_time - scan_span.
      // We also adjust the cur_time and byterate values here so the next
      // iteration can be a bit more precise.
      if (packet->pts > seek_time || packet->pts < seek_time - scan_span) {
        found_packet_with_pts = TRUE;
        byterate = (seek_pos - cur_pos) / (packet->pts - cur_time);
        cur_time = packet->pts;
        break;
      }

      // If we are still here, it means this packet is in close range to
      // the seek_time. If this is the first packet for this jump position
      // record the PTS. If we later have to back off, when there was no
      // intra frame in this range, we can lower the seek_time to not scan
      // this range again.
      if (!found_packet_in_range) {
        found_packet_in_range = TRUE;
        first_packet_time = packet->pts;
      }

      // Check if this is an intra frame packet. If so, record the buffer
      // position of the start of this packet. We want to jump back to it
      // later, when we know it's the last intra frame before desired
      // seek time.
      if (force_intra) {
        for (size_t i = 0; i < packet->length - 6; i++) {
          // Find the START_PICTURE code
          if (packet->data[i] == 0x00 && packet->data[i + 1] == 0x00 &&
              packet->data[i + 2] == 0x01 && packet->data[i + 3] == 0x00) {
            // Bits 11--13 in the picture header contain the frame
            // type, where 1=Intra
            if ((packet->data[i + 5] & 0x38) == 8) {
              last_valid_packet_start = packet_start;
            }
            break;
          }
        }
      }

      // If we don't want intra frames, just use the last PTS found.
      else {
        last_valid_packet_start = packet_start;
      }
    }

    // If there was at least one intra frame in the range scanned above,
    // our search is over. Jump back to the packet and decode it again.
    if (last_valid_packet_start != -1) {
      plm_demux_buffer_seek(self, last_valid_packet_start);
      return plm_demux_decode_packet(self, type);
    }

    // If we hit the right range, but still found no intra frame, we have
    // to increases the scan_span. This is done exponentially to also handle
    // video files with very few intra frames.
    else if (found_packet_in_range) {
      scan_span *= 2;
      seek_time = first_packet_time;
    }

    // If we didn't find any packet with a PTS, it probably means we reached
    // the end of the file. Estimate byterate and cur_time accordingly.
    else if (!found_packet_with_pts) {
      byterate = (seek_pos - cur_pos) / (duration - cur_time);
      cur_time = duration;
    }
  }

  return NULL;
}

plm_packet_t *plm_demux_decode(plm_demux_t *self) {
  if (!plm_demux_has_headers(self)) {
    return NULL;
  }

  if (self->current_packet.length) {
    size_t bits_till_next_packet = self->current_packet.length << 3;
    if (!plm_buffer_has(self->buffer, bits_till_next_packet)) {
      return NULL;
    }
    plm_buffer_skip(self->buffer, bits_till_next_packet);
    self->current_packet.length = 0;
  }

  // Pending packet waiting for data?
  if (self->next_packet.length) {
    return plm_demux_get_packet(self);
  }

  // Pending packet waiting for header?
  if (self->start_code != -1) {
    return plm_demux_decode_packet(self, self->start_code);
  }

  do {
    self->start_code = plm_buffer_next_start_code(self->buffer);
    if (self->start_code == PLM_DEMUX_PACKET_VIDEO_1 ||
        self->start_code == PLM_DEMUX_PACKET_PRIVATE ||
        (self->start_code >= PLM_DEMUX_PACKET_AUDIO_1 &&
         self->start_code <= PLM_DEMUX_PACKET_AUDIO_4)) {
      return plm_demux_decode_packet(self, self->start_code);
    }
  } while (self->start_code != -1);

  return NULL;
}

double plm_demux_decode_time(plm_demux_t *self) {
  int64_t clock = plm_buffer_read(self->buffer, 3) << 30;
  plm_buffer_skip(self->buffer, 1);
  clock |= plm_buffer_read(self->buffer, 15) << 15;
  plm_buffer_skip(self->buffer, 1);
  clock |= plm_buffer_read(self->buffer, 15);
  plm_buffer_skip(self->buffer, 1);
  return (double)clock / 90000.0;
}

plm_packet_t *plm_demux_decode_packet(plm_demux_t *self, int type) {
  if (!plm_buffer_has(self->buffer, 16 << 3)) {
    return NULL;
  }

  self->start_code = -1;

  self->next_packet.type = type;
  self->next_packet.length = plm_buffer_read(self->buffer, 16);
  self->next_packet.length -=
      plm_buffer_skip_bytes(self->buffer, 0xff);  // stuffing

  // skip P-STD
  if (plm_buffer_read(self->buffer, 2) == 0x01) {
    plm_buffer_skip(self->buffer, 16);
    self->next_packet.length -= 2;
  }

  int pts_dts_marker = plm_buffer_read(self->buffer, 2);
  if (pts_dts_marker == 0x03) {
    self->next_packet.pts = plm_demux_decode_time(self);
    self->last_decoded_pts = self->next_packet.pts;
    plm_buffer_skip(self->buffer, 40);  // skip dts
    self->next_packet.length -= 10;
  } else if (pts_dts_marker == 0x02) {
    self->next_packet.pts = plm_demux_decode_time(self);
    self->last_decoded_pts = self->next_packet.pts;
    self->next_packet.length -= 5;
  } else if (pts_dts_marker == 0x00) {
    self->next_packet.pts = PLM_PACKET_INVALID_TS;
    plm_buffer_skip(self->buffer, 4);
    self->next_packet.length -= 1;
  } else {
    return NULL;  // invalid
  }

  return plm_demux_get_packet(self);
}

plm_packet_t *plm_demux_get_packet(plm_demux_t *self) {
  if (!plm_buffer_has(self->buffer, self->next_packet.length << 3)) {
    return NULL;
  }

  self->current_packet.data =
      self->buffer->bytes + (self->buffer->bit_index >> 3);
  self->current_packet.length = self->next_packet.length;
  self->current_packet.type = self->next_packet.type;
  self->current_packet.pts = self->next_packet.pts;

  self->next_packet.length = 0;
  return &self->current_packet;
}


plm_video_t *plm_video_create_with_buffer(plm_buffer_t *buffer,
                                          int destroy_when_done) {
  plm_video_t *self = &static_video_holder;
  memset(self, 0, sizeof(plm_video_t));

  self->buffer = buffer;
  self->destroy_buffer_when_done = destroy_when_done;

  // Attempt to decode the sequence header
  self->start_code =
      plm_buffer_find_start_code(self->buffer, PLM_START_SEQUENCE);
  if (self->start_code != -1) {
    plm_video_decode_sequence_header(self);
  }
  return self;
}

void plm_video_destroy(plm_video_t *self) {
  if (self->destroy_buffer_when_done) {
    plm_buffer_destroy(self->buffer);
  }
}

double plm_video_get_framerate(plm_video_t *self) {
  return plm_video_has_header(self) ? self->framerate : 0;
}

double plm_video_get_pixel_aspect_ratio(plm_video_t *self) {
  return plm_video_has_header(self) ? self->pixel_aspect_ratio : 0;
}

int plm_video_get_width(plm_video_t *self) {
  return plm_video_has_header(self) ? self->width : 0;
}

int plm_video_get_height(plm_video_t *self) {
  return plm_video_has_header(self) ? self->height : 0;
}

void plm_video_set_no_delay(plm_video_t *self, int no_delay) {
  self->assume_no_b_frames = no_delay;
}

double plm_video_get_time(plm_video_t *self) { return self->time; }

void plm_video_set_time(plm_video_t *self, double time) {
  self->frames_decoded = self->framerate * time;
  self->time = time;
}

void plm_video_rewind(plm_video_t *self) {
  plm_buffer_rewind(self->buffer);
  self->time = 0;
  self->frames_decoded = 0;
  self->has_reference_frame = FALSE;
  self->start_code = -1;
}

int plm_video_has_ended(plm_video_t *self) {
  return plm_buffer_has_ended(self->buffer);
}

plm_frame_t *plm_video_decode(plm_video_t *self) {
  if (!plm_video_has_header(self)) {
    return NULL;
  }

  plm_frame_t *frame = NULL;
  do {
    if (self->start_code != PLM_START_PICTURE) {
      self->start_code =
          plm_buffer_find_start_code(self->buffer, PLM_START_PICTURE);

      if (self->start_code == -1) {
        // If we reached the end of the file and the previously decoded
        // frame was a reference frame, we still have to return it.
        if (self->has_reference_frame && !self->assume_no_b_frames &&
            plm_buffer_has_ended(self->buffer) &&
            (self->picture_type == PLM_VIDEO_PICTURE_TYPE_INTRA ||
             self->picture_type == PLM_VIDEO_PICTURE_TYPE_PREDICTIVE)) {
          self->has_reference_frame = FALSE;
          frame = &self->frame_backward;
          break;
        }

        return NULL;
      }
    }

    // Make sure we have a full picture in the buffer before attempting to
    // decode it. Sadly, this can only be done by seeking for the start code
    // of the next picture. Also, if we didn't find the start code for the
    // next picture, but the source has ended, we assume that this last
    // picture is in the buffer.
    if (plm_buffer_has_start_code(self->buffer, PLM_START_PICTURE) == -1 &&
        !plm_buffer_has_ended(self->buffer)) {
      return NULL;
    }
    plm_buffer_discard_read_bytes(self->buffer);

    plm_video_decode_picture(self);

    if (self->assume_no_b_frames) {
      frame = &self->frame_backward;
    } else if (self->picture_type == PLM_VIDEO_PICTURE_TYPE_B) {
      frame = &self->frame_current;
    } else if (self->has_reference_frame) {
      frame = &self->frame_forward;
    } else {
      self->has_reference_frame = TRUE;
    }
  } while (!frame);

  frame->time = self->time;
  self->frames_decoded++;
  self->time = (double)self->frames_decoded / self->framerate;

  return frame;
}

int plm_video_has_header(plm_video_t *self) {
  if (self->has_sequence_header) {
    return TRUE;
  }

  if (self->start_code != PLM_START_SEQUENCE) {
    self->start_code =
        plm_buffer_find_start_code(self->buffer, PLM_START_SEQUENCE);
  }
  if (self->start_code == -1) {
    return FALSE;
  }

  if (!plm_video_decode_sequence_header(self)) {
    return FALSE;
  }

  return TRUE;
}

// uint8_t g_frames_data[18446744];

int plm_video_decode_sequence_header(plm_video_t *self) {
  int max_header_size = 64 + 2 * 64 * 8;  // 64 bit header + 2x 64 byte matrix
  if (!plm_buffer_has(self->buffer, max_header_size)) {
    return FALSE;
  }

  self->width = plm_buffer_read(self->buffer, 12);
  self->height = plm_buffer_read(self->buffer, 12);

  if (self->width <= 0 || self->height <= 0) {
    return FALSE;
  }

  // Get pixel aspect ratio
  int pixel_aspect_ratio_code;
  pixel_aspect_ratio_code = plm_buffer_read(self->buffer, 4);
  pixel_aspect_ratio_code -= 1;
  if (pixel_aspect_ratio_code < 0) {
    pixel_aspect_ratio_code = 0;
  }
  int par_last = (sizeof(PLM_VIDEO_PIXEL_ASPECT_RATIO) /
                      sizeof(PLM_VIDEO_PIXEL_ASPECT_RATIO[0]) -
                  1);
  if (pixel_aspect_ratio_code > par_last) {
    pixel_aspect_ratio_code = par_last;
  }
  self->pixel_aspect_ratio =
      PLM_VIDEO_PIXEL_ASPECT_RATIO[pixel_aspect_ratio_code];

  // Get frame rate
  self->framerate = PLM_VIDEO_PICTURE_RATE[plm_buffer_read(self->buffer, 4)];

  // Skip bit_rate, marker, buffer_size and constrained bit
  plm_buffer_skip(self->buffer, 18 + 1 + 10 + 1);

  // Load custom intra quant matrix?
  if (plm_buffer_read(self->buffer, 1)) {
    for (int i = 0; i < 64; i++) {
      int idx = PLM_VIDEO_ZIG_ZAG[i];
      self->intra_quant_matrix[idx] = plm_buffer_read(self->buffer, 8);
    }
  } else {
    memcpy(self->intra_quant_matrix, PLM_VIDEO_INTRA_QUANT_MATRIX, 64);
  }

  // Load custom non intra quant matrix?
  if (plm_buffer_read(self->buffer, 1)) {
    for (int i = 0; i < 64; i++) {
      int idx = PLM_VIDEO_ZIG_ZAG[i];
      self->non_intra_quant_matrix[idx] = plm_buffer_read(self->buffer, 8);
    }
  } else {
    memcpy(self->non_intra_quant_matrix, PLM_VIDEO_NON_INTRA_QUANT_MATRIX, 64);
  }

  self->mb_width = (self->width + 15) >> 4;
  self->mb_height = (self->height + 15) >> 4;
  self->mb_size = self->mb_width * self->mb_height;

  self->luma_width = self->mb_width << 4;
  self->luma_height = self->mb_height << 4;

  self->chroma_width = self->mb_width << 3;
  self->chroma_height = self->mb_height << 3;

  // Allocate one big chunk of data for all 3 frames = 9 planes
  size_t luma_plane_size = self->luma_width * self->luma_height;
  size_t chroma_plane_size = self->chroma_width * self->chroma_height;
  size_t frame_data_size = (luma_plane_size + 2 * chroma_plane_size);

  plm_video_init_frame(self, &self->frame_current,
                       self->frames_data + frame_data_size * 0);
  plm_video_init_frame(self, &self->frame_forward,
                       self->frames_data + frame_data_size * 1);
  plm_video_init_frame(self, &self->frame_backward,
                       self->frames_data + frame_data_size * 2);

  self->has_sequence_header = TRUE;
  return TRUE;
}

void plm_video_init_frame(plm_video_t *self, plm_frame_t *frame,
                          uint8_t *base) {
  size_t luma_plane_size = self->luma_width * self->luma_height;
  size_t chroma_plane_size = self->chroma_width * self->chroma_height;

  frame->width = self->width;
  frame->height = self->height;
  frame->y.width = self->luma_width;
  frame->y.height = self->luma_height;
  frame->y.data = base;

  frame->cr.width = self->chroma_width;
  frame->cr.height = self->chroma_height;
  frame->cr.data = base + luma_plane_size;

  frame->cb.width = self->chroma_width;
  frame->cb.height = self->chroma_height;
  frame->cb.data = base + luma_plane_size + chroma_plane_size;
}

void plm_video_decode_picture(plm_video_t *self) {
  plm_buffer_skip(self->buffer, 10);  // skip temporalReference
  self->picture_type = plm_buffer_read(self->buffer, 3);
  plm_buffer_skip(self->buffer, 16);  // skip vbv_delay

  // D frames or unknown coding type
  if (self->picture_type <= 0 ||
      self->picture_type > PLM_VIDEO_PICTURE_TYPE_B) {
    return;
  }

  // Forward full_px, f_code
  if (self->picture_type == PLM_VIDEO_PICTURE_TYPE_PREDICTIVE ||
      self->picture_type == PLM_VIDEO_PICTURE_TYPE_B) {
    self->motion_forward.full_px = plm_buffer_read(self->buffer, 1);
    int f_code = plm_buffer_read(self->buffer, 3);
    if (f_code == 0) {
      // Ignore picture with zero f_code
      return;
    }
    self->motion_forward.r_size = f_code - 1;
  }

  // Backward full_px, f_code
  if (self->picture_type == PLM_VIDEO_PICTURE_TYPE_B) {
    self->motion_backward.full_px = plm_buffer_read(self->buffer, 1);
    int f_code = plm_buffer_read(self->buffer, 3);
    if (f_code == 0) {
      // Ignore picture with zero f_code
      return;
    }
    self->motion_backward.r_size = f_code - 1;
  }

  plm_frame_t frame_temp = self->frame_forward;
  if (self->picture_type == PLM_VIDEO_PICTURE_TYPE_INTRA ||
      self->picture_type == PLM_VIDEO_PICTURE_TYPE_PREDICTIVE) {
    self->frame_forward = self->frame_backward;
  }

  // Find first slice start code; skip extension and user data
  do {
    self->start_code = plm_buffer_next_start_code(self->buffer);
  } while (self->start_code == PLM_START_EXTENSION ||
           self->start_code == PLM_START_USER_DATA);

  // Decode all slices
  while (PLM_START_IS_SLICE(self->start_code)) {
    plm_video_decode_slice(self, self->start_code & 0x000000FF);
    if (self->macroblock_address >= self->mb_size - 2) {
      break;
    }
    self->start_code = plm_buffer_next_start_code(self->buffer);
  }

  // If this is a reference picture rotate the prediction pointers
  if (self->picture_type == PLM_VIDEO_PICTURE_TYPE_INTRA ||
      self->picture_type == PLM_VIDEO_PICTURE_TYPE_PREDICTIVE) {
    self->frame_backward = self->frame_current;
    self->frame_current = frame_temp;
  }
}

void plm_video_decode_slice(plm_video_t *self, int slice) {
  self->slice_begin = TRUE;
  self->macroblock_address = (slice - 1) * self->mb_width - 1;

  // Reset motion vectors and DC predictors
  self->motion_backward.h = self->motion_forward.h = 0;
  self->motion_backward.v = self->motion_forward.v = 0;
  self->dc_predictor[0] = 128;
  self->dc_predictor[1] = 128;
  self->dc_predictor[2] = 128;

  self->quantizer_scale = plm_buffer_read(self->buffer, 5);

  // Skip extra
  while (plm_buffer_read(self->buffer, 1)) {
    plm_buffer_skip(self->buffer, 8);
  }

  do {
    plm_video_decode_macroblock(self);
  } while (self->macroblock_address < self->mb_size - 1 &&
           plm_buffer_peek_non_zero(self->buffer, 23));
}

void plm_video_decode_macroblock(plm_video_t *self) {
  // Decode increment
  int increment = 0;
  int t =
      plm_buffer_read_vlc(self->buffer, PLM_VIDEO_MACROBLOCK_ADDRESS_INCREMENT);

  while (t == 34) {
    // macroblock_stuffing
    t = plm_buffer_read_vlc(self->buffer,
                            PLM_VIDEO_MACROBLOCK_ADDRESS_INCREMENT);
  }
  while (t == 35) {
    // macroblock_escape
    increment += 33;
    t = plm_buffer_read_vlc(self->buffer,
                            PLM_VIDEO_MACROBLOCK_ADDRESS_INCREMENT);
  }
  increment += t;

  // Process any skipped macroblocks
  if (self->slice_begin) {
    // The first increment of each slice is relative to beginning of the
    // previous row, not the previous macroblock
    self->slice_begin = FALSE;
    self->macroblock_address += increment;
  } else {
    if (self->macroblock_address + increment >= self->mb_size) {
      return;  // invalid
    }
    if (increment > 1) {
      // Skipped macroblocks reset DC predictors
      self->dc_predictor[0] = 128;
      self->dc_predictor[1] = 128;
      self->dc_predictor[2] = 128;

      // Skipped macroblocks in P-pictures reset motion vectors
      if (self->picture_type == PLM_VIDEO_PICTURE_TYPE_PREDICTIVE) {
        self->motion_forward.h = 0;
        self->motion_forward.v = 0;
      }
    }

    // Predict skipped macroblocks
    while (increment > 1) {
      self->macroblock_address++;
      self->mb_row = self->macroblock_address / self->mb_width;
      self->mb_col = self->macroblock_address % self->mb_width;

      plm_video_predict_macroblock(self);
      increment--;
    }
    self->macroblock_address++;
  }

  self->mb_row = self->macroblock_address / self->mb_width;
  self->mb_col = self->macroblock_address % self->mb_width;

  if (self->mb_col >= self->mb_width || self->mb_row >= self->mb_height) {
    return;  // corrupt stream;
  }

  // Process the current macroblock
  const plm_vlc_t *table = PLM_VIDEO_MACROBLOCK_TYPE[self->picture_type];
  self->macroblock_type = plm_buffer_read_vlc(self->buffer, table);

  self->macroblock_intra = (self->macroblock_type & 0x01);
  self->motion_forward.is_set = (self->macroblock_type & 0x08);
  self->motion_backward.is_set = (self->macroblock_type & 0x04);

  // Quantizer scale
  if ((self->macroblock_type & 0x10) != 0) {
    self->quantizer_scale = plm_buffer_read(self->buffer, 5);
  }

  if (self->macroblock_intra) {
    // Intra-coded macroblocks reset motion vectors
    self->motion_backward.h = self->motion_forward.h = 0;
    self->motion_backward.v = self->motion_forward.v = 0;
  } else {
    // Non-intra macroblocks reset DC predictors
    self->dc_predictor[0] = 128;
    self->dc_predictor[1] = 128;
    self->dc_predictor[2] = 128;

    plm_video_decode_motion_vectors(self);
    plm_video_predict_macroblock(self);
  }

  // Decode blocks
  int cbp =
      ((self->macroblock_type & 0x02) != 0)
          ? plm_buffer_read_vlc(self->buffer, PLM_VIDEO_CODE_BLOCK_PATTERN)
          : (self->macroblock_intra ? 0x3f : 0);

  for (int block = 0, mask = 0x20; block < 6; block++) {
    if ((cbp & mask) != 0) {
      plm_video_decode_block(self, block);
    }
    mask >>= 1;
  }
}

void plm_video_decode_motion_vectors(plm_video_t *self) {
  // Forward
  if (self->motion_forward.is_set) {
    int r_size = self->motion_forward.r_size;
    self->motion_forward.h =
        plm_video_decode_motion_vector(self, r_size, self->motion_forward.h);
    self->motion_forward.v =
        plm_video_decode_motion_vector(self, r_size, self->motion_forward.v);
  } else if (self->picture_type == PLM_VIDEO_PICTURE_TYPE_PREDICTIVE) {
    // No motion information in P-picture, reset vectors
    self->motion_forward.h = 0;
    self->motion_forward.v = 0;
  }

  if (self->motion_backward.is_set) {
    int r_size = self->motion_backward.r_size;
    self->motion_backward.h =
        plm_video_decode_motion_vector(self, r_size, self->motion_backward.h);
    self->motion_backward.v =
        plm_video_decode_motion_vector(self, r_size, self->motion_backward.v);
  }
}

int plm_video_decode_motion_vector(plm_video_t *self, int r_size, int motion) {
  int fscale = 1 << r_size;
  int m_code = plm_buffer_read_vlc(self->buffer, PLM_VIDEO_MOTION);
  int r = 0;
  int d;

  if ((m_code != 0) && (fscale != 1)) {
    r = plm_buffer_read(self->buffer, r_size);
    d = ((etl::absolute(m_code) - 1) << r_size) + r + 1;
    if (m_code < 0) {
      d = -d;
    }
  } else {
    d = m_code;
  }

  motion += d;
  if (motion > (fscale << 4) - 1) {
    motion -= fscale << 5;
  } else if (motion < (int)((unsigned)(-fscale) << 4)) {
    motion += fscale << 5;
  }

  return motion;
}

void plm_video_predict_macroblock(plm_video_t *self) {
  int fw_h = self->motion_forward.h;
  int fw_v = self->motion_forward.v;

  if (self->motion_forward.full_px) {
    fw_h <<= 1;
    fw_v <<= 1;
  }

  if (self->picture_type == PLM_VIDEO_PICTURE_TYPE_B) {
    int bw_h = self->motion_backward.h;
    int bw_v = self->motion_backward.v;

    if (self->motion_backward.full_px) {
      bw_h <<= 1;
      bw_v <<= 1;
    }

    if (self->motion_forward.is_set) {
      plm_video_copy_macroblock(self, &self->frame_forward, fw_h, fw_v);
      if (self->motion_backward.is_set) {
        plm_video_interpolate_macroblock(self, &self->frame_backward, bw_h,
                                         bw_v);
      }
    } else {
      plm_video_copy_macroblock(self, &self->frame_backward, bw_h, bw_v);
    }
  } else {
    plm_video_copy_macroblock(self, &self->frame_forward, fw_h, fw_v);
  }
}

void plm_video_copy_macroblock(plm_video_t *self, plm_frame_t *s, int motion_h,
                               int motion_v) {
  plm_frame_t *d = &self->frame_current;
  plm_video_process_macroblock(self, s->y.data, d->y.data, motion_h, motion_v,
                               16, FALSE);
  plm_video_process_macroblock(self, s->cr.data, d->cr.data, motion_h / 2,
                               motion_v / 2, 8, FALSE);
  plm_video_process_macroblock(self, s->cb.data, d->cb.data, motion_h / 2,
                               motion_v / 2, 8, FALSE);
}

void plm_video_interpolate_macroblock(plm_video_t *self, plm_frame_t *s,
                                      int motion_h, int motion_v) {
  plm_frame_t *d = &self->frame_current;
  plm_video_process_macroblock(self, s->y.data, d->y.data, motion_h, motion_v,
                               16, TRUE);
  plm_video_process_macroblock(self, s->cr.data, d->cr.data, motion_h / 2,
                               motion_v / 2, 8, TRUE);
  plm_video_process_macroblock(self, s->cb.data, d->cb.data, motion_h / 2,
                               motion_v / 2, 8, TRUE);
}


void plm_video_process_macroblock(plm_video_t *self, uint8_t *s, uint8_t *d,
                                  int motion_h, int motion_v, int block_size,
                                  int interpolate) {
  int dw = self->mb_width * block_size;

  int hp = motion_h >> 1;
  int vp = motion_v >> 1;
  int odd_h = (motion_h & 1) == 1;
  int odd_v = (motion_v & 1) == 1;

  unsigned int si = ((self->mb_row * block_size) + vp) * dw +
                    (self->mb_col * block_size) + hp;
  unsigned int di = (self->mb_row * dw + self->mb_col) * block_size;

  unsigned int max_address =
      (dw * (self->mb_height * block_size - block_size + 1) - block_size);
  if (si > max_address || di > max_address) {
    return;  // corrupt video
  }

#define PLM_MB_CASE(INTERPOLATE, ODD_H, ODD_V, OP)    \
  case ((INTERPOLATE << 2) | (ODD_H << 1) | (ODD_V)): \
    PLM_BLOCK_SET(d, di, dw, si, dw, block_size, OP); \
    break

  switch ((interpolate << 2) | (odd_h << 1) | (odd_v)) {
    PLM_MB_CASE(0, 0, 0, (s[si]));
    PLM_MB_CASE(0, 0, 1, (s[si] + s[si + dw] + 1) >> 1);
    PLM_MB_CASE(0, 1, 0, (s[si] + s[si + 1] + 1) >> 1);
    PLM_MB_CASE(0, 1, 1,
                (s[si] + s[si + 1] + s[si + dw] + s[si + dw + 1] + 2) >> 2);

    PLM_MB_CASE(1, 0, 0, (d[di] + (s[si]) + 1) >> 1);
    PLM_MB_CASE(1, 0, 1, (d[di] + ((s[si] + s[si + dw] + 1) >> 1) + 1) >> 1);
    PLM_MB_CASE(1, 1, 0, (d[di] + ((s[si] + s[si + 1] + 1) >> 1) + 1) >> 1);
    PLM_MB_CASE(
        1, 1, 1,
        (d[di] + ((s[si] + s[si + 1] + s[si + dw] + s[si + dw + 1] + 2) >> 2) +
         1) >>
            1);
  }

#undef PLM_MB_CASE
}

void plm_video_decode_block(plm_video_t *self, int block) {
  int n = 0;
  uint8_t *quant_matrix;

  // Decode DC coefficient of intra-coded blocks
  if (self->macroblock_intra) {
    int predictor;
    int dct_size;

    // DC prediction
    int plane_index = block > 3 ? block - 3 : 0;
    predictor = self->dc_predictor[plane_index];
    dct_size =
        plm_buffer_read_vlc(self->buffer, PLM_VIDEO_DCT_SIZE[plane_index]);

    // Read DC coeff
    if (dct_size > 0) {
      int differential = plm_buffer_read(self->buffer, dct_size);
      if ((differential & (1 << (dct_size - 1))) != 0) {
        self->block_data[0] = predictor + differential;
      } else {
        self->block_data[0] =
            predictor + (-(1 << dct_size) | (differential + 1));
      }
    } else {
      self->block_data[0] = predictor;
    }

    // Save predictor value
    self->dc_predictor[plane_index] = self->block_data[0];

    // Dequantize + premultiply
    self->block_data[0] <<= (3 + 5);

    quant_matrix = self->intra_quant_matrix;
    n = 1;
  } else {
    quant_matrix = self->non_intra_quant_matrix;
  }

  // Decode AC coefficients (+DC for non-intra)
  int level = 0;
  while (TRUE) {
    int run = 0;
    uint16_t coeff =
        plm_buffer_read_vlc_uint(self->buffer, PLM_VIDEO_DCT_COEFF);

    if ((coeff == 0x0001) && (n > 0) &&
        (plm_buffer_read(self->buffer, 1) == 0)) {
      // end_of_block
      break;
    }
    if (coeff == 0xffff) {
      // escape
      run = plm_buffer_read(self->buffer, 6);
      level = plm_buffer_read(self->buffer, 8);
      if (level == 0) {
        level = plm_buffer_read(self->buffer, 8);
      } else if (level == 128) {
        level = plm_buffer_read(self->buffer, 8) - 256;
      } else if (level > 128) {
        level = level - 256;
      }
    } else {
      run = coeff >> 8;
      level = coeff & 0xff;
      if (plm_buffer_read(self->buffer, 1)) {
        level = -level;
      }
    }

    n += run;
    if (n < 0 || n >= 64) {
      return;  // invalid
    }

    int de_zig_zagged = PLM_VIDEO_ZIG_ZAG[n];
    n++;

    // Dequantize, oddify, clip
    level = (unsigned)level << 1;
    if (!self->macroblock_intra) {
      level += (level < 0 ? -1 : 1);
    }
    level = (level * self->quantizer_scale * quant_matrix[de_zig_zagged]) >> 4;
    if ((level & 1) == 0) {
      level -= level > 0 ? 1 : -1;
    }
    if (level > 2047) {
      level = 2047;
    } else if (level < -2048) {
      level = -2048;
    }

    // Save premultiplied coefficient
    self->block_data[de_zig_zagged] =
        level * PLM_VIDEO_PREMULTIPLIER_MATRIX[de_zig_zagged];
  }

  // Move block to its place
  uint8_t *d;
  int dw;
  int di;

  if (block < 4) {
    d = self->frame_current.y.data;
    dw = self->luma_width;
    di = (self->mb_row * self->luma_width + self->mb_col) << 4;
    if ((block & 1) != 0) {
      di += 8;
    }
    if ((block & 2) != 0) {
      di += self->luma_width << 3;
    }
  } else {
    d = (block == 4) ? self->frame_current.cb.data
                     : self->frame_current.cr.data;
    dw = self->chroma_width;
    di = ((self->mb_row * self->luma_width) << 2) + (self->mb_col << 3);
  }

  int *s = self->block_data;
  int si = 0;
  if (self->macroblock_intra) {
    // Overwrite (no prediction)
    if (n == 1) {
      int clamped = plm_clamp((s[0] + 128) >> 8);
      PLM_BLOCK_SET(d, di, dw, si, 8, 8, clamped);
      s[0] = 0;
    } else {
      plm_video_idct(s);
      PLM_BLOCK_SET(d, di, dw, si, 8, 8, plm_clamp(s[si]));
      memset(self->block_data, 0, sizeof(self->block_data));
    }
  } else {
    // Add data to the predicted macroblock
    if (n == 1) {
      int value = (s[0] + 128) >> 8;
      PLM_BLOCK_SET(d, di, dw, si, 8, 8, plm_clamp(d[di] + value));
      s[0] = 0;
    } else {
      plm_video_idct(s);
      PLM_BLOCK_SET(d, di, dw, si, 8, 8, plm_clamp(d[di] + s[si]));
      memset(self->block_data, 0, sizeof(self->block_data));
    }
  }
}

void plm_video_idct(int *block) {
  int b1, b3, b4, b6, b7, tmp1, tmp2, m0, x0, x1, x2, x3, x4, y3, y4, y5, y6,
      y7;

  // Transform columns
  for (int i = 0; i < 8; ++i) {
    b1 = block[4 * 8 + i];
    b3 = block[2 * 8 + i] + block[6 * 8 + i];
    b4 = block[5 * 8 + i] - block[3 * 8 + i];
    tmp1 = block[1 * 8 + i] + block[7 * 8 + i];
    tmp2 = block[3 * 8 + i] + block[5 * 8 + i];
    b6 = block[1 * 8 + i] - block[7 * 8 + i];
    b7 = tmp1 + tmp2;
    m0 = block[0 * 8 + i];
    x4 = ((b6 * 473 - b4 * 196 + 128) >> 8) - b7;
    x0 = x4 - (((tmp1 - tmp2) * 362 + 128) >> 8);
    x1 = m0 - b1;
    x2 = (((block[2 * 8 + i] - block[6 * 8 + i]) * 362 + 128) >> 8) - b3;
    x3 = m0 + b1;
    y3 = x1 + x2;
    y4 = x3 + b3;
    y5 = x1 - x2;
    y6 = x3 - b3;
    y7 = -x0 - ((b4 * 473 + b6 * 196 + 128) >> 8);
    block[0 * 8 + i] = b7 + y4;
    block[1 * 8 + i] = x4 + y3;
    block[2 * 8 + i] = y5 - x0;
    block[3 * 8 + i] = y6 - y7;
    block[4 * 8 + i] = y6 + y7;
    block[5 * 8 + i] = x0 + y5;
    block[6 * 8 + i] = y3 - x4;
    block[7 * 8 + i] = y4 - b7;
  }

  // Transform rows
  for (int i = 0; i < 64; i += 8) {
    b1 = block[4 + i];
    b3 = block[2 + i] + block[6 + i];
    b4 = block[5 + i] - block[3 + i];
    tmp1 = block[1 + i] + block[7 + i];
    tmp2 = block[3 + i] + block[5 + i];
    b6 = block[1 + i] - block[7 + i];
    b7 = tmp1 + tmp2;
    m0 = block[0 + i];
    x4 = ((b6 * 473 - b4 * 196 + 128) >> 8) - b7;
    x0 = x4 - (((tmp1 - tmp2) * 362 + 128) >> 8);
    x1 = m0 - b1;
    x2 = (((block[2 + i] - block[6 + i]) * 362 + 128) >> 8) - b3;
    x3 = m0 + b1;
    y3 = x1 + x2;
    y4 = x3 + b3;
    y5 = x1 - x2;
    y6 = x3 - b3;
    y7 = -x0 - ((b4 * 473 + b6 * 196 + 128) >> 8);
    block[0 + i] = (b7 + y4 + 128) >> 8;
    block[1 + i] = (x4 + y3 + 128) >> 8;
    block[2 + i] = (y5 - x0 + 128) >> 8;
    block[3 + i] = (y6 - y7 + 128) >> 8;
    block[4 + i] = (y6 + y7 + 128) >> 8;
    block[5 + i] = (x0 + y5 + 128) >> 8;
    block[6 + i] = (y3 - x4 + 128) >> 8;
    block[7 + i] = (y4 - b7 + 128) >> 8;
  }
}

PLM_DEFINE_FRAME_CONVERT_FUNCTION(plm_frame_to_rgb, 3, 0, 1, 2)
PLM_DEFINE_FRAME_CONVERT_FUNCTION(plm_frame_to_bgr, 3, 2, 1, 0)
PLM_DEFINE_FRAME_CONVERT_FUNCTION(plm_frame_to_rgba, 4, 0, 1, 2)
PLM_DEFINE_FRAME_CONVERT_FUNCTION(plm_frame_to_bgra, 4, 2, 1, 0)
PLM_DEFINE_FRAME_CONVERT_FUNCTION(plm_frame_to_argb, 4, 1, 2, 3)
PLM_DEFINE_FRAME_CONVERT_FUNCTION(plm_frame_to_abgr, 4, 3, 2, 1)

#undef PLM_PUT_PIXEL
#undef PLM_DEFINE_FRAME_CONVERT_FUNCTION


plm_audio_t *plm_audio_create_with_buffer(plm_buffer_t *buffer,
                                          int destroy_when_done) {
  plm_audio_t *self = &static_audio_holder;
  memset(self, 0, sizeof(plm_audio_t));

  self->samples.count = PLM_AUDIO_SAMPLES_PER_FRAME;
  self->buffer = buffer;
  self->destroy_buffer_when_done = destroy_when_done;
  self->samplerate_index = 3;  // Indicates 0

  memcpy(self->D, PLM_AUDIO_SYNTHESIS_WINDOW, 512 * sizeof(float));
  memcpy(self->D + 512, PLM_AUDIO_SYNTHESIS_WINDOW, 512 * sizeof(float));

  // Attempt to decode first header
  self->next_frame_data_size = plm_audio_decode_header(self);

  return self;
}

void plm_audio_destroy(plm_audio_t *self) {
  if (self->destroy_buffer_when_done) {
    plm_buffer_destroy(self->buffer);
  }
}

int plm_audio_has_header(plm_audio_t *self) {
  if (self->has_header) {
    return TRUE;
  }

  self->next_frame_data_size = plm_audio_decode_header(self);
  return self->has_header;
}

int plm_audio_get_samplerate(plm_audio_t *self) {
  return plm_audio_has_header(self)
             ? PLM_AUDIO_SAMPLE_RATE[self->samplerate_index]
             : 0;
}

double plm_audio_get_time(plm_audio_t *self) { return self->time; }

void plm_audio_set_time(plm_audio_t *self, double time) {
  self->samples_decoded =
      time * (double)PLM_AUDIO_SAMPLE_RATE[self->samplerate_index];
  self->time = time;
}

void plm_audio_rewind(plm_audio_t *self) {
  plm_buffer_rewind(self->buffer);
  self->time = 0;
  self->samples_decoded = 0;
  self->next_frame_data_size = 0;
}

int plm_audio_has_ended(plm_audio_t *self) {
  return plm_buffer_has_ended(self->buffer);
}

plm_samples_t *plm_audio_decode(plm_audio_t *self) {
  // Do we have at least enough information to decode the frame header?
  if (!self->next_frame_data_size) {
    if (!plm_buffer_has(self->buffer, 48)) {
      return NULL;
    }
    self->next_frame_data_size = plm_audio_decode_header(self);
  }

  if (self->next_frame_data_size == 0 ||
      !plm_buffer_has(self->buffer, self->next_frame_data_size << 3)) {
    return NULL;
  }

  plm_audio_decode_frame(self);
  self->next_frame_data_size = 0;

  self->samples.time = self->time;

  self->samples_decoded += PLM_AUDIO_SAMPLES_PER_FRAME;
  self->time = (double)self->samples_decoded /
               (double)PLM_AUDIO_SAMPLE_RATE[self->samplerate_index];

  return &self->samples;
}

int plm_audio_find_frame_sync(plm_audio_t *self) {
  size_t i;
  for (i = self->buffer->bit_index >> 3; i < self->buffer->length - 1; i++) {
    if (self->buffer->bytes[i] == 0xFF &&
        (self->buffer->bytes[i + 1] & 0xFE) == 0xFC) {
      self->buffer->bit_index = ((i + 1) << 3) + 3;
      return TRUE;
    }
  }
  self->buffer->bit_index = (i + 1) << 3;
  return FALSE;
}

int plm_audio_decode_header(plm_audio_t *self) {
  if (!plm_buffer_has(self->buffer, 48)) {
    return 0;
  }

  plm_buffer_skip_bytes(self->buffer, 0x00);
  int sync = plm_buffer_read(self->buffer, 11);

  // Attempt to resync if no syncword was found. This sucks balls. The MP2
  // stream contains a syncword just before every frame (11 bits set to 1).
  // However, this syncword is not guaranteed to not occur elsewhere in the
  // stream. So, if we have to resync, we also have to check if the header
  // (samplerate, bitrate) differs from the one we had before. This all
  // may still lead to garbage data being decoded :/

  if (sync != PLM_AUDIO_FRAME_SYNC && !plm_audio_find_frame_sync(self)) {
    return 0;
  }

  self->version = plm_buffer_read(self->buffer, 2);
  self->layer = plm_buffer_read(self->buffer, 2);
  int hasCRC = !plm_buffer_read(self->buffer, 1);

  if (self->version != PLM_AUDIO_MPEG_1 || self->layer != PLM_AUDIO_LAYER_II) {
    return 0;
  }

  int bitrate_index = plm_buffer_read(self->buffer, 4) - 1;
  if (bitrate_index > 13) {
    return 0;
  }

  int samplerate_index = plm_buffer_read(self->buffer, 2);
  if (samplerate_index == 3) {
    return 0;
  }

  int padding = plm_buffer_read(self->buffer, 1);
  plm_buffer_skip(self->buffer, 1);  // f_private
  int mode = plm_buffer_read(self->buffer, 2);

  // If we already have a header, make sure the samplerate, bitrate and mode
  // are still the same, otherwise we might have missed sync.
  if (self->has_header &&
      (self->bitrate_index != bitrate_index ||
       self->samplerate_index != samplerate_index || self->mode != mode)) {
    return 0;
  }

  self->bitrate_index = bitrate_index;
  self->samplerate_index = samplerate_index;
  self->mode = mode;
  self->has_header = TRUE;

  // Parse the mode_extension, set up the stereo bound
  if (mode == PLM_AUDIO_MODE_JOINT_STEREO) {
    self->bound = (plm_buffer_read(self->buffer, 2) + 1) << 2;
  } else {
    plm_buffer_skip(self->buffer, 2);
    self->bound = (mode == PLM_AUDIO_MODE_MONO) ? 0 : 32;
  }

  // Discard the last 4 bits of the header and the CRC value, if present
  plm_buffer_skip(self->buffer, 4);  // copyright(1), original(1), emphasis(2)
  if (hasCRC) {
    plm_buffer_skip(self->buffer, 16);
  }

  // Compute frame size, check if we have enough data to decode the whole
  // frame.
  int bitrate = PLM_AUDIO_BIT_RATE[self->bitrate_index];
  int samplerate = PLM_AUDIO_SAMPLE_RATE[self->samplerate_index];
  int frame_size = (144000 * bitrate / samplerate) + padding;
  return frame_size - (hasCRC ? 6 : 4);
}

void plm_audio_decode_frame(plm_audio_t *self) {
  // Prepare the quantizer table lookups
  int tab3 = 0;
  int sblimit = 0;

  int tab1 = (self->mode == PLM_AUDIO_MODE_MONO) ? 0 : 1;
  int tab2 = PLM_AUDIO_QUANT_LUT_STEP_1[tab1][self->bitrate_index];
  tab3 = QUANT_LUT_STEP_2[tab2][self->samplerate_index];
  sblimit = tab3 & 63;
  tab3 >>= 6;

  if (self->bound > sblimit) {
    self->bound = sblimit;
  }

  // Read the allocation information
  for (int sb = 0; sb < self->bound; sb++) {
    self->allocation[0][sb] = plm_audio_read_allocation(self, sb, tab3);
    self->allocation[1][sb] = plm_audio_read_allocation(self, sb, tab3);
  }

  for (int sb = self->bound; sb < sblimit; sb++) {
    self->allocation[0][sb] = self->allocation[1][sb] =
        plm_audio_read_allocation(self, sb, tab3);
  }

  // Read scale factor selector information
  int channels = (self->mode == PLM_AUDIO_MODE_MONO) ? 1 : 2;
  for (int sb = 0; sb < sblimit; sb++) {
    for (int ch = 0; ch < channels; ch++) {
      if (self->allocation[ch][sb]) {
        self->scale_factor_info[ch][sb] = plm_buffer_read(self->buffer, 2);
      }
    }
    if (self->mode == PLM_AUDIO_MODE_MONO) {
      self->scale_factor_info[1][sb] = self->scale_factor_info[0][sb];
    }
  }

  // Read scale factors
  for (int sb = 0; sb < sblimit; sb++) {
    for (int ch = 0; ch < channels; ch++) {
      if (self->allocation[ch][sb]) {
        int *sf = self->scale_factor[ch][sb];
        switch (self->scale_factor_info[ch][sb]) {
          case 0:
            sf[0] = plm_buffer_read(self->buffer, 6);
            sf[1] = plm_buffer_read(self->buffer, 6);
            sf[2] = plm_buffer_read(self->buffer, 6);
            break;
          case 1:
            sf[0] = sf[1] = plm_buffer_read(self->buffer, 6);
            sf[2] = plm_buffer_read(self->buffer, 6);
            break;
          case 2:
            sf[0] = sf[1] = sf[2] = plm_buffer_read(self->buffer, 6);
            break;
          case 3:
            sf[0] = plm_buffer_read(self->buffer, 6);
            sf[1] = sf[2] = plm_buffer_read(self->buffer, 6);
            break;
        }
      }
    }
    if (self->mode == PLM_AUDIO_MODE_MONO) {
      self->scale_factor[1][sb][0] = self->scale_factor[0][sb][0];
      self->scale_factor[1][sb][1] = self->scale_factor[0][sb][1];
      self->scale_factor[1][sb][2] = self->scale_factor[0][sb][2];
    }
  }

  // Coefficient input and reconstruction
  int out_pos = 0;
  for (int part = 0; part < 3; part++) {
    for (int granule = 0; granule < 4; granule++) {
      // Read the samples
      for (int sb = 0; sb < self->bound; sb++) {
        plm_audio_read_samples(self, 0, sb, part);
        plm_audio_read_samples(self, 1, sb, part);
      }
      for (int sb = self->bound; sb < sblimit; sb++) {
        plm_audio_read_samples(self, 0, sb, part);
        self->sample[1][sb][0] = self->sample[0][sb][0];
        self->sample[1][sb][1] = self->sample[0][sb][1];
        self->sample[1][sb][2] = self->sample[0][sb][2];
      }
      for (int sb = sblimit; sb < 32; sb++) {
        self->sample[0][sb][0] = 0;
        self->sample[0][sb][1] = 0;
        self->sample[0][sb][2] = 0;
        self->sample[1][sb][0] = 0;
        self->sample[1][sb][1] = 0;
        self->sample[1][sb][2] = 0;
      }

      // Synthesis loop
      for (int p = 0; p < 3; p++) {
        // Shifting step
        self->v_pos = (self->v_pos - 64) & 1023;

        for (int ch = 0; ch < 2; ch++) {
          plm_audio_idct36(self->sample[ch], p, self->V[ch], self->v_pos);

          // Build U, windowing, calculate output
          memset(self->U, 0, sizeof(self->U));

          int d_index = 512 - (self->v_pos >> 1);
          int v_index = (self->v_pos % 128) >> 1;
          while (v_index < 1024) {
            for (int i = 0; i < 32; ++i) {
              self->U[i] += self->D[d_index++] * self->V[ch][v_index++];
            }

            v_index += 128 - 32;
            d_index += 64 - 32;
          }

          d_index -= (512 - 32);
          v_index = (128 - 32 + 1024) - v_index;
          while (v_index < 1024) {
            for (int i = 0; i < 32; ++i) {
              self->U[i] += self->D[d_index++] * self->V[ch][v_index++];
            }

            v_index += 128 - 32;
            d_index += 64 - 32;
          }

// Output samples
#ifdef PLM_AUDIO_SEPARATE_CHANNELS
          float *out_channel =
              ch == 0 ? self->samples.left : self->samples.right;
          for (int j = 0; j < 32; j++) {
            out_channel[out_pos + j] = self->U[j] / 2147418112.0f;
          }
#else
          for (int j = 0; j < 32; j++) {
            self->samples.interleaved[((out_pos + j) << 1) + ch] =
                self->U[j] / 2147418112.0f;
          }
#endif
        }  // End of synthesis channel loop
        out_pos += 32;
      }  // End of synthesis sub-block loop

    }  // Decoding of the granule finished
  }

  plm_buffer_align(self->buffer);
}

const plm_quantizer_spec_t *plm_audio_read_allocation(plm_audio_t *self, int sb,
                                                      int tab3) {
  int tab4 = PLM_AUDIO_QUANT_LUT_STEP_3[tab3][sb];
  int qtab = PLM_AUDIO_QUANT_LUT_STEP_4[tab4 & 15][plm_buffer_read(self->buffer,
                                                                   tab4 >> 4)];
  return qtab ? (&PLM_AUDIO_QUANT_TAB[qtab - 1]) : 0;
}

void plm_audio_read_samples(plm_audio_t *self, int ch, int sb, int part) {
  const plm_quantizer_spec_t *q = self->allocation[ch][sb];
  int sf = self->scale_factor[ch][sb][part];
  int *sample = self->sample[ch][sb];
  int val = 0;

  if (!q) {
    // No bits allocated for this subband
    sample[0] = sample[1] = sample[2] = 0;
    return;
  }

  // Resolve scalefactor
  if (sf == 63) {
    sf = 0;
  } else {
    int shift = (sf / 3) | 0;
    sf = (PLM_AUDIO_SCALEFACTOR_BASE[sf % 3] + ((1 << shift) >> 1)) >> shift;
  }

  // Decode samples
  int adj = q->levels;
  if (q->group) {
    // Decode grouped samples
    val = plm_buffer_read(self->buffer, q->bits);
    sample[0] = val % adj;
    val /= adj;
    sample[1] = val % adj;
    sample[2] = val / adj;
  } else {
    // Decode direct samples
    sample[0] = plm_buffer_read(self->buffer, q->bits);
    sample[1] = plm_buffer_read(self->buffer, q->bits);
    sample[2] = plm_buffer_read(self->buffer, q->bits);
  }

  // Postmultiply samples
  int scale = 65536 / (adj + 1);
  adj = ((adj + 1) >> 1) - 1;

  val = (adj - sample[0]) * scale;
  sample[0] = (val * (sf >> 12) + ((val * (sf & 4095) + 2048) >> 12)) >> 12;

  val = (adj - sample[1]) * scale;
  sample[1] = (val * (sf >> 12) + ((val * (sf & 4095) + 2048) >> 12)) >> 12;

  val = (adj - sample[2]) * scale;
  sample[2] = (val * (sf >> 12) + ((val * (sf & 4095) + 2048) >> 12)) >> 12;
}

void plm_audio_idct36(int s[32][3], int ss, float *d, int dp) {
  float t01, t02, t03, t04, t05, t06, t07, t08, t09, t10, t11, t12, t13, t14,
      t15, t16, t17, t18, t19, t20, t21, t22, t23, t24, t25, t26, t27, t28, t29,
      t30, t31, t32, t33;

  t01 = (float)(s[0][ss] + s[31][ss]);
  t02 = (float)(s[0][ss] - s[31][ss]) * 0.500602998235f;
  t03 = (float)(s[1][ss] + s[30][ss]);
  t04 = (float)(s[1][ss] - s[30][ss]) * 0.505470959898f;
  t05 = (float)(s[2][ss] + s[29][ss]);
  t06 = (float)(s[2][ss] - s[29][ss]) * 0.515447309923f;
  t07 = (float)(s[3][ss] + s[28][ss]);
  t08 = (float)(s[3][ss] - s[28][ss]) * 0.53104259109f;
  t09 = (float)(s[4][ss] + s[27][ss]);
  t10 = (float)(s[4][ss] - s[27][ss]) * 0.553103896034f;
  t11 = (float)(s[5][ss] + s[26][ss]);
  t12 = (float)(s[5][ss] - s[26][ss]) * 0.582934968206f;
  t13 = (float)(s[6][ss] + s[25][ss]);
  t14 = (float)(s[6][ss] - s[25][ss]) * 0.622504123036f;
  t15 = (float)(s[7][ss] + s[24][ss]);
  t16 = (float)(s[7][ss] - s[24][ss]) * 0.674808341455f;
  t17 = (float)(s[8][ss] + s[23][ss]);
  t18 = (float)(s[8][ss] - s[23][ss]) * 0.744536271002f;
  t19 = (float)(s[9][ss] + s[22][ss]);
  t20 = (float)(s[9][ss] - s[22][ss]) * 0.839349645416f;
  t21 = (float)(s[10][ss] + s[21][ss]);
  t22 = (float)(s[10][ss] - s[21][ss]) * 0.972568237862f;
  t23 = (float)(s[11][ss] + s[20][ss]);
  t24 = (float)(s[11][ss] - s[20][ss]) * 1.16943993343f;
  t25 = (float)(s[12][ss] + s[19][ss]);
  t26 = (float)(s[12][ss] - s[19][ss]) * 1.48416461631f;
  t27 = (float)(s[13][ss] + s[18][ss]);
  t28 = (float)(s[13][ss] - s[18][ss]) * 2.05778100995f;
  t29 = (float)(s[14][ss] + s[17][ss]);
  t30 = (float)(s[14][ss] - s[17][ss]) * 3.40760841847f;
  t31 = (float)(s[15][ss] + s[16][ss]);
  t32 = (float)(s[15][ss] - s[16][ss]) * 10.1900081235f;

  t33 = t01 + t31;
  t31 = (t01 - t31) * 0.502419286188f;
  t01 = t03 + t29;
  t29 = (t03 - t29) * 0.52249861494f;
  t03 = t05 + t27;
  t27 = (t05 - t27) * 0.566944034816f;
  t05 = t07 + t25;
  t25 = (t07 - t25) * 0.64682178336f;
  t07 = t09 + t23;
  t23 = (t09 - t23) * 0.788154623451f;
  t09 = t11 + t21;
  t21 = (t11 - t21) * 1.06067768599f;
  t11 = t13 + t19;
  t19 = (t13 - t19) * 1.72244709824f;
  t13 = t15 + t17;
  t17 = (t15 - t17) * 5.10114861869f;
  t15 = t33 + t13;
  t13 = (t33 - t13) * 0.509795579104f;
  t33 = t01 + t11;
  t01 = (t01 - t11) * 0.601344886935f;
  t11 = t03 + t09;
  t09 = (t03 - t09) * 0.899976223136f;
  t03 = t05 + t07;
  t07 = (t05 - t07) * 2.56291544774f;
  t05 = t15 + t03;
  t15 = (t15 - t03) * 0.541196100146f;
  t03 = t33 + t11;
  t11 = (t33 - t11) * 1.30656296488f;
  t33 = t05 + t03;
  t05 = (t05 - t03) * 0.707106781187f;
  t03 = t15 + t11;
  t15 = (t15 - t11) * 0.707106781187f;
  t03 += t15;
  t11 = t13 + t07;
  t13 = (t13 - t07) * 0.541196100146f;
  t07 = t01 + t09;
  t09 = (t01 - t09) * 1.30656296488f;
  t01 = t11 + t07;
  t07 = (t11 - t07) * 0.707106781187f;
  t11 = t13 + t09;
  t13 = (t13 - t09) * 0.707106781187f;
  t11 += t13;
  t01 += t11;
  t11 += t07;
  t07 += t13;
  t09 = t31 + t17;
  t31 = (t31 - t17) * 0.509795579104f;
  t17 = t29 + t19;
  t29 = (t29 - t19) * 0.601344886935f;
  t19 = t27 + t21;
  t21 = (t27 - t21) * 0.899976223136f;
  t27 = t25 + t23;
  t23 = (t25 - t23) * 2.56291544774f;
  t25 = t09 + t27;
  t09 = (t09 - t27) * 0.541196100146f;
  t27 = t17 + t19;
  t19 = (t17 - t19) * 1.30656296488f;
  t17 = t25 + t27;
  t27 = (t25 - t27) * 0.707106781187f;
  t25 = t09 + t19;
  t19 = (t09 - t19) * 0.707106781187f;
  t25 += t19;
  t09 = t31 + t23;
  t31 = (t31 - t23) * 0.541196100146f;
  t23 = t29 + t21;
  t21 = (t29 - t21) * 1.30656296488f;
  t29 = t09 + t23;
  t23 = (t09 - t23) * 0.707106781187f;
  t09 = t31 + t21;
  t31 = (t31 - t21) * 0.707106781187f;
  t09 += t31;
  t29 += t09;
  t09 += t23;
  t23 += t31;
  t17 += t29;
  t29 += t25;
  t25 += t09;
  t09 += t27;
  t27 += t23;
  t23 += t19;
  t19 += t31;
  t21 = t02 + t32;
  t02 = (t02 - t32) * 0.502419286188f;
  t32 = t04 + t30;
  t04 = (t04 - t30) * 0.52249861494f;
  t30 = t06 + t28;
  t28 = (t06 - t28) * 0.566944034816f;
  t06 = t08 + t26;
  t08 = (t08 - t26) * 0.64682178336f;
  t26 = t10 + t24;
  t10 = (t10 - t24) * 0.788154623451f;
  t24 = t12 + t22;
  t22 = (t12 - t22) * 1.06067768599f;
  t12 = t14 + t20;
  t20 = (t14 - t20) * 1.72244709824f;
  t14 = t16 + t18;
  t16 = (t16 - t18) * 5.10114861869f;
  t18 = t21 + t14;
  t14 = (t21 - t14) * 0.509795579104f;
  t21 = t32 + t12;
  t32 = (t32 - t12) * 0.601344886935f;
  t12 = t30 + t24;
  t24 = (t30 - t24) * 0.899976223136f;
  t30 = t06 + t26;
  t26 = (t06 - t26) * 2.56291544774f;
  t06 = t18 + t30;
  t18 = (t18 - t30) * 0.541196100146f;
  t30 = t21 + t12;
  t12 = (t21 - t12) * 1.30656296488f;
  t21 = t06 + t30;
  t30 = (t06 - t30) * 0.707106781187f;
  t06 = t18 + t12;
  t12 = (t18 - t12) * 0.707106781187f;
  t06 += t12;
  t18 = t14 + t26;
  t26 = (t14 - t26) * 0.541196100146f;
  t14 = t32 + t24;
  t24 = (t32 - t24) * 1.30656296488f;
  t32 = t18 + t14;
  t14 = (t18 - t14) * 0.707106781187f;
  t18 = t26 + t24;
  t24 = (t26 - t24) * 0.707106781187f;
  t18 += t24;
  t32 += t18;
  t18 += t14;
  t26 = t14 + t24;
  t14 = t02 + t16;
  t02 = (t02 - t16) * 0.509795579104f;
  t16 = t04 + t20;
  t04 = (t04 - t20) * 0.601344886935f;
  t20 = t28 + t22;
  t22 = (t28 - t22) * 0.899976223136f;
  t28 = t08 + t10;
  t10 = (t08 - t10) * 2.56291544774f;
  t08 = t14 + t28;
  t14 = (t14 - t28) * 0.541196100146f;
  t28 = t16 + t20;
  t20 = (t16 - t20) * 1.30656296488f;
  t16 = t08 + t28;
  t28 = (t08 - t28) * 0.707106781187f;
  t08 = t14 + t20;
  t20 = (t14 - t20) * 0.707106781187f;
  t08 += t20;
  t14 = t02 + t10;
  t02 = (t02 - t10) * 0.541196100146f;
  t10 = t04 + t22;
  t22 = (t04 - t22) * 1.30656296488f;
  t04 = t14 + t10;
  t10 = (t14 - t10) * 0.707106781187f;
  t14 = t02 + t22;
  t02 = (t02 - t22) * 0.707106781187f;
  t14 += t02;
  t04 += t14;
  t14 += t10;
  t10 += t02;
  t16 += t04;
  t04 += t08;
  t08 += t14;
  t14 += t28;
  t28 += t10;
  t10 += t20;
  t20 += t02;
  t21 += t16;
  t16 += t32;
  t32 += t04;
  t04 += t06;
  t06 += t08;
  t08 += t18;
  t18 += t14;
  t14 += t30;
  t30 += t28;
  t28 += t26;
  t26 += t10;
  t10 += t12;
  t12 += t20;
  t20 += t24;
  t24 += t02;

  d[dp + 48] = -t33;
  d[dp + 49] = d[dp + 47] = -t21;
  d[dp + 50] = d[dp + 46] = -t17;
  d[dp + 51] = d[dp + 45] = -t16;
  d[dp + 52] = d[dp + 44] = -t01;
  d[dp + 53] = d[dp + 43] = -t32;
  d[dp + 54] = d[dp + 42] = -t29;
  d[dp + 55] = d[dp + 41] = -t04;
  d[dp + 56] = d[dp + 40] = -t03;
  d[dp + 57] = d[dp + 39] = -t06;
  d[dp + 58] = d[dp + 38] = -t25;
  d[dp + 59] = d[dp + 37] = -t08;
  d[dp + 60] = d[dp + 36] = -t11;
  d[dp + 61] = d[dp + 35] = -t18;
  d[dp + 62] = d[dp + 34] = -t09;
  d[dp + 63] = d[dp + 33] = -t14;
  d[dp + 32] = -t05;
  d[dp + 0] = t05;
  d[dp + 31] = -t30;
  d[dp + 1] = t30;
  d[dp + 30] = -t27;
  d[dp + 2] = t27;
  d[dp + 29] = -t28;
  d[dp + 3] = t28;
  d[dp + 28] = -t07;
  d[dp + 4] = t07;
  d[dp + 27] = -t26;
  d[dp + 5] = t26;
  d[dp + 26] = -t23;
  d[dp + 6] = t23;
  d[dp + 25] = -t10;
  d[dp + 7] = t10;
  d[dp + 24] = -t15;
  d[dp + 8] = t15;
  d[dp + 23] = -t12;
  d[dp + 9] = t12;
  d[dp + 22] = -t19;
  d[dp + 10] = t19;
  d[dp + 21] = -t20;
  d[dp + 11] = t20;
  d[dp + 20] = -t13;
  d[dp + 12] = t13;
  d[dp + 19] = -t24;
  d[dp + 13] = t24;
  d[dp + 18] = -t31;
  d[dp + 14] = t31;
  d[dp + 17] = -t02;
  d[dp + 15] = t02;
  d[dp + 16] = 0.0;
}
