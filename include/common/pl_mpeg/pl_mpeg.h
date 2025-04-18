/*
PL_MPEG - MPEG1 Video decoder, MP2 Audio decoder, MPEG-PS demuxer

Dominic Szablewski - https://phoboslab.org


-- LICENSE: The MIT License(MIT)

Copyright(c) 2019 Dominic Szablewski

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files(the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and / or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions :
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.




-- Synopsis

// Define `PL_MPEG_IMPLEMENTATION` in *one* C/C++ file before including this
// library to create the implementation.

#define PL_MPEG_IMPLEMENTATION
#include "plmpeg.h"

// This function gets called for each decoded video frame
void my_video_callback(plm_t *plm, plm_frame_t *frame, void *user) {
        // Do something with frame->y.data, frame->cr.data, frame->cb.data
}

// This function gets called for each decoded audio frame
void my_audio_callback(plm_t *plm, plm_samples_t *frame, void *user) {
        // Do something with samples->interleaved
}

// Load a .mpg (MPEG Program Stream) file
plm_t *plm = plm_create_with_filename("some-file.mpg");

// Install the video & audio decode callbacks
plm_set_video_decode_callback(plm, my_video_callback, my_data);
plm_set_audio_decode_callback(plm, my_audio_callback, my_data);


// Decode
do {
        plm_decode(plm, time_since_last_call);
} while (!plm_has_ended(plm));

// All done
plm_destroy(plm);



-- Documentation

This library provides several interfaces to load, demux and decode MPEG video
and audio data. A high-level API combines the demuxer, video & audio decoders
in an easy to use wrapper.

Lower-level APIs for accessing the demuxer, video decoder and audio decoder,
as well as providing different data sources are also available.

Interfaces are written in an object oriented style, meaning you create object
instances via various different constructor functions (plm_*create()),
do some work on them and later dispose them via plm_*destroy().

plm_* ......... the high-level interface, combining demuxer and decoders
plm_buffer_* .. the data source used by all interfaces
plm_demux_* ... the MPEG-PS demuxer
plm_video_* ... the MPEG1 Video ("mpeg1") decoder
plm_audio_* ... the MPEG1 Audio Layer II ("mp2") decoder


With the high-level interface you have two options to decode video & audio:

 1. Use plm_decode() and just hand over the delta time since the last call.
    It will decode everything needed and call your callbacks (specified through
    plm_set_{video|audio}_decode_callback()) any number of times.

 2. Use plm_decode_video() and plm_decode_audio() to decode exactly one
    frame of video or audio data at a time. How you handle the synchronization
    of both streams is up to you.

If you only want to decode video *or* audio through these functions, you should
disable the other stream (plm_set_{video|audio}_enabled(FALSE))

Video data is decoded into a struct with all 3 planes (Y, Cr, Cb) stored in
separate buffers. You can either convert this to RGB on the CPU (slow) via the
plm_frame_to_rgb() function or do it on the GPU with the following matrix:

mat4 bt601 = mat4(
        1.16438,  0.00000,  1.59603, -0.87079,
        1.16438, -0.39176, -0.81297,  0.52959,
        1.16438,  2.01723,  0.00000, -1.08139,
        0, 0, 0, 1
);
gl_FragColor = vec4(y, cb, cr, 1.0) * bt601;

Audio data is decoded into a struct with either one single float array with the
samples for the left and right channel interleaved, or if the
PLM_AUDIO_SEPARATE_CHANNELS is defined *before* including this library, into
two separate float arrays - one for each channel.


Data can be supplied to the high level interface, the demuxer and the decoders
in three different ways:

 1. Using plm_create_from_filename() or with a file handle with
    plm_create_from_file().

 2. Using plm_create_with_memory() and supplying a pointer to memory that
    contains the whole file.

 3. Using plm_create_with_buffer(), supplying your own plm_buffer_t instance and
    periodically writing to this buffer.

When using your own plm_buffer_t instance, you can fill this buffer using
plm_buffer_write(). You can either monitor plm_buffer_get_remaining() and push
data when appropriate, or install a callback on the buffer with
plm_buffer_set_load_callback() that gets called whenever the buffer needs more
data.

A buffer created with plm_buffer_create_with_capacity() is treated as a ring
buffer, meaning that data that has already been read, will be discarded. In
contrast, a buffer created with plm_buffer_create_for_appending() will keep all
data written to it in memory. This enables seeking in the already loaded data.


There should be no need to use the lower level plm_demux_*, plm_video_* and
plm_audio_* functions, if all you want to do is read/decode an MPEG-PS file.
However, if you get raw mpeg1video data or raw mp2 audio data from a different
source, these functions can be used to decode the raw data directly. Similarly,
if you only want to analyze an MPEG-PS file or extract raw video or audio
packets from it, you can use the plm_demux_* functions.


This library uses malloc(), realloc() and free() to manage memory. Typically
all allocation happens up-front when creating the interface. However, the
default buffer size may be too small for certain inputs. In these cases plmpeg
will realloc() the buffer with a larger size whenever needed. You can configure
the default buffer size by defining PLM_BUFFER_DEFAULT_SIZE *before*
including this library.

See below for detailed the API documentation.

*/

// #ifndef PL_MPEG_H
// #define PL_MPEG_H
// #if __STDC_HOSTED__ == 1
// #include <bits/types/FILE.h>
// #else
// #pragma message("compiling on bm")
// #include <stdio.h>
// #endif
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

// -----------------------------------------------------------------------------
// Public Data Types

// Object types for the various interfaces

typedef struct plm_t plm_t;
typedef struct plm_buffer_t plm_buffer_t;
typedef struct plm_demux_t plm_demux_t;
typedef struct plm_video_t plm_video_t;
typedef struct plm_audio_t plm_audio_t;

// Demuxed MPEG PS packet
// The type maps directly to the various MPEG-PES start codes. PTS is the
// presentation time stamp of the packet in seconds. Note that not all packets
// have a PTS value, indicated by PLM_PACKET_INVALID_TS.

#define PLM_PACKET_INVALID_TS -1

typedef struct {
  int type;
  double pts;
  size_t length;
  uint8_t *data;
} plm_packet_t;

// Decoded Video Plane
// The byte length of the data is width * height. Note that different planes
// have different sizes: the Luma plane (Y) is double the size of each of
// the two Chroma planes (Cr, Cb) - i.e. 4 times the byte length.
// Also note that the size of the plane does *not* denote the size of the
// displayed frame. The sizes of planes are always rounded up to the nearest
// macroblock (16px).

typedef struct {
  unsigned int width;
  unsigned int height;
  uint8_t *data;
} plm_plane_t;

// Decoded Video Frame
// width and height denote the desired display size of the frame. This may be
// different from the internal size of the 3 planes.

typedef struct {
  double time;
  unsigned int width;
  unsigned int height;
  plm_plane_t y;
  plm_plane_t cr;
  plm_plane_t cb;
} plm_frame_t;

// Callback function type for decoded video frames used by the high-level
// plm_* interface

typedef void (*plm_video_decode_callback)(plm_t *self, plm_frame_t *frame,
                                          void *user);

// Decoded Audio Samples
// Samples are stored as normalized (-1, 1) float either interleaved, or if
// PLM_AUDIO_SEPARATE_CHANNELS is defined, in two separate arrays.
// The `count` is always PLM_AUDIO_SAMPLES_PER_FRAME and just there for
// convenience.

#define PLM_AUDIO_SAMPLES_PER_FRAME 1152

typedef struct {
  double time;
  unsigned int count;
#ifdef PLM_AUDIO_SEPARATE_CHANNELS
  float left[PLM_AUDIO_SAMPLES_PER_FRAME];
  float right[PLM_AUDIO_SAMPLES_PER_FRAME];
#else
  float interleaved[PLM_AUDIO_SAMPLES_PER_FRAME * 2];
#endif
} plm_samples_t;

// Callback function type for decoded audio samples used by the high-level
// plm_* interface

typedef void (*plm_audio_decode_callback)(plm_t *self, plm_samples_t *samples,
                                          void *user);

// Callback function for plm_buffer when it needs more data

typedef void (*plm_buffer_load_callback)(plm_buffer_t *self, void *user);

// -----------------------------------------------------------------------------
// plm_* public API
// High-Level API for loading/demuxing/decoding MPEG-PS data

// Create a plmpeg instance with a filename. Returns NULL if the file could
// not be opened.

plm_t *plm_create_with_filename(const char *filename, plm_t *self_ptr);

// Create a plmpeg instance with a file handle. Pass TRUE to close_when_done
// to let plmpeg call fclose() on the handle when plm_destroy() is called.

plm_t *plm_create_with_file(FILE *fh, int close_when_done, plm_t *self_ptr);

// Create a plmpeg instance with a pointer to memory as source. This assumes
// the whole file is in memory. The memory is not copied. Pass TRUE to
// free_when_done to let plmpeg call free() on the pointer when plm_destroy()
// is called.

plm_t *plm_create_with_memory(uint8_t *bytes, size_t length, int free_when_done,
                              plm_t *self_ptr);

// Create a plmpeg instance with a plm_buffer as source. Pass TRUE to
// destroy_when_done to let plmpeg call plm_buffer_destroy() on the buffer
// when plm_destroy() is called.

void plm_create_with_buffer(plm_buffer_t *buffer, int destroy_when_done,
                            plm_t *self_ptr);

// Destroy a plmpeg instance and free all data.

void plm_destroy(plm_t *self);

// Get whether we have headers on all available streams and we can report the
// number of video/audio streams, video dimensions, framerate and audio
// samplerate.
// This returns FALSE if the file is not an MPEG-PS file or - when not using a
// file as source - when not enough data is available yet.

int plm_has_headers(plm_t *self);

// Probe the MPEG-PS data to find the actual number of video and audio streams
// within the buffer. For certain files (e.g. VideoCD) this can be more
// accurate than just reading the number of streams from the headers. This
// should only be used when the underlying plm_buffer is seekable, i.e. for
// files, fixed memory buffers or _for_appending buffers. If used with dynamic
// memory buffers it will skip decoding the probesize!
// The necessary probesize is dependent on the files you expect to read.
// Usually a few hundred KB should be enough to find all streams. Use
// plm_get_num_{audio|video}_streams() afterwards to get the number of streams
// in the file. Returns TRUE if any streams were found within the probesize.

int plm_probe(plm_t *self, size_t probesize);

// Get or set whether video decoding is enabled. Default TRUE.

int plm_get_video_enabled(plm_t *self);
void plm_set_video_enabled(plm_t *self, int enabled);

// Get the number of video streams (0--1) reported in the system header.

int plm_get_num_video_streams(plm_t *self);

// Get the display width/height of the video stream.

int plm_get_width(plm_t *self);
int plm_get_height(plm_t *self);
double plm_get_pixel_aspect_ratio(plm_t *self);

// Get the framerate of the video stream in frames per second.

double plm_get_framerate(plm_t *self);

// Get or set whether audio decoding is enabled. Default TRUE.

int plm_get_audio_enabled(plm_t *self);
void plm_set_audio_enabled(plm_t *self, int enabled);

// Get the number of audio streams (0--4) reported in the system header.

int plm_get_num_audio_streams(plm_t *self);

// Set the desired audio stream (0--3). Default 0.

void plm_set_audio_stream(plm_t *self, int stream_index);

// Get the samplerate of the audio stream in samples per second.

int plm_get_samplerate(plm_t *self);

// Get or set the audio lead time in seconds - the time in which audio samples
// are decoded in advance (or behind) the video decode time. Typically this
// should be set to the duration of the buffer of the audio API that you use
// for output. E.g. for SDL2: (SDL_AudioSpec.samples / samplerate)

double plm_get_audio_lead_time(plm_t *self);
void plm_set_audio_lead_time(plm_t *self, double lead_time);

// Get the current internal time in seconds.

double plm_get_time(plm_t *self);

// Get the video duration of the underlying source in seconds.

double plm_get_duration(plm_t *self);

// Rewind all buffers back to the beginning.

void plm_rewind(plm_t *self);

// Get or set looping. Default FALSE.

int plm_get_loop(plm_t *self);
void plm_set_loop(plm_t *self, int loop);

// Get whether the file has ended. If looping is enabled, this will always
// return FALSE.

int plm_has_ended(plm_t *self);

// Set the callback for decoded video frames used with plm_decode(). If no
// callback is set, video data will be ignored and not be decoded. The *user
// Parameter will be passed to your callback.

void plm_set_video_decode_callback(plm_t *self, plm_video_decode_callback fp,
                                   void *user);

// Set the callback for decoded audio samples used with plm_decode(). If no
// callback is set, audio data will be ignored and not be decoded. The *user
// Parameter will be passed to your callback.

void plm_set_audio_decode_callback(plm_t *self, plm_audio_decode_callback fp,
                                   void *user);

// Advance the internal timer by seconds and decode video/audio up to this
// time. This will call the video_decode_callback and audio_decode_callback
// any number of times. A frame-skip is not implemented, i.e. everything up to
// current time will be decoded.

void plm_decode(plm_t *self, double seconds);

// Decode and return one video frame. Returns NULL if no frame could be
// decoded (either because the source ended or data is corrupt). If you only
// want to decode video, you should disable audio via plm_set_audio_enabled().
// The returned plm_frame_t is valid until the next call to plm_decode_video()
// or until plm_destroy() is called.

plm_frame_t *plm_decode_video(plm_t *self);

// Decode and return one audio frame. Returns NULL if no frame could be
// decoded (either because the source ended or data is corrupt). If you only
// want to decode audio, you should disable video via plm_set_video_enabled().
// The returned plm_samples_t is valid until the next call to
// plm_decode_audio() or until plm_destroy() is called.

plm_samples_t *plm_decode_audio(plm_t *self);

// Seek to the specified time, clamped between 0 -- duration. This can only be
// used when the underlying plm_buffer is seekable, i.e. for files, fixed
// memory buffers or _for_appending buffers.
// If seek_exact is TRUE this will seek to the exact time, otherwise it will
// seek to the last intra frame just before the desired time. Exact seeking
// can be slow, because all frames up to the seeked one have to be decoded on
// top of the previous intra frame. If seeking succeeds, this function will
// call the video_decode_callback exactly once with the target frame. If audio
// is enabled, it will also call the audio_decode_callback any number of
// times, until the audio_lead_time is satisfied. Returns TRUE if seeking
// succeeded or FALSE if no frame could be found.

int plm_seek(plm_t *self, double time, int seek_exact);

// Similar to plm_seek(), but will not call the video_decode_callback,
// audio_decode_callback or make any attempts to sync audio.
// Returns the found frame or NULL if no frame could be found.

plm_frame_t *plm_seek_frame(plm_t *self, double time, int seek_exact);

// -----------------------------------------------------------------------------
// plm_buffer public API
// Provides the data source for all other plm_* interfaces

// The default size for buffers created from files or by the high-level API

#ifndef PLM_BUFFER_DEFAULT_SIZE
//#define PLM_BUFFER_DEFAULT_SIZE ((128 * 1024) * 6)
#define PLM_BUFFER_DEFAULT_SIZE 15970304
#endif

// Create a buffer instance with a filename. Returns NULL if the file could
// not be opened.

plm_buffer_t *plm_buffer_create_with_filename(const char *filename);

// Create a buffer instance with a file handle. Pass TRUE to close_when_done
// to let plmpeg call fclose() on the handle when plm_destroy() is called.

plm_buffer_t *plm_buffer_create_with_file(FILE *fh, int close_when_done);

// Create a buffer instance with a pointer to memory as source. This assumes
// the whole file is in memory. The bytes are not copied. Pass 1 to
// free_when_done to let plmpeg call free() on the pointer when plm_destroy()
// is called.

plm_buffer_t *plm_buffer_create_with_memory(uint8_t *bytes, size_t length,
                                            int free_when_done);

// Create an empty buffer with an initial capacity. The buffer will grow
// as needed. Data that has already been read, will be discarded.

plm_buffer_t *plm_buffer_create_with_capacity(size_t capacity);

// Create an empty buffer with an initial capacity. The buffer will grow
// as needed. Decoded data will *not* be discarded. This can be used when
// loading a file over the network, without needing to throttle the download.
// It also allows for seeking in the already loaded data.

plm_buffer_t *plm_buffer_create_for_appending(size_t initial_capacity);

// Destroy a buffer instance and free all data

void plm_buffer_destroy(plm_buffer_t *self);

// Copy data into the buffer. If the data to be written is larger than the
// available space, the buffer will realloc() with a larger capacity.
// Returns the number of bytes written. This will always be the same as the
// passed in length, except when the buffer was created _with_memory() for
// which _write() is forbidden.

size_t plm_buffer_write(plm_buffer_t *self, uint8_t *bytes, size_t length);

// Mark the current byte length as the end of this buffer and signal that no
// more data is expected to be written to it. This function should be called
// just after the last plm_buffer_write().
// For _with_capacity buffers, this is cleared on a plm_buffer_rewind().

void plm_buffer_signal_end(plm_buffer_t *self);

// Set a callback that is called whenever the buffer needs more data

void plm_buffer_set_load_callback(plm_buffer_t *self,
                                  plm_buffer_load_callback fp, void *user);

// Rewind the buffer back to the beginning. When loading from a file handle,
// this also seeks to the beginning of the file.

void plm_buffer_rewind(plm_buffer_t *self);

// Get the total size. For files, this returns the file size. For all other
// types it returns the number of bytes currently in the buffer.

size_t plm_buffer_get_size(plm_buffer_t *self);

// Get the number of remaining (yet unread) bytes in the buffer. This can be
// useful to throttle writing.

size_t plm_buffer_get_remaining(plm_buffer_t *self);

// Get whether the read position of the buffer is at the end and no more data
// is expected.

int plm_buffer_has_ended(plm_buffer_t *self);

// -----------------------------------------------------------------------------
// plm_demux public API
// Demux an MPEG Program Stream (PS) data into separate packages

// Various Packet Types

static const int PLM_DEMUX_PACKET_PRIVATE = 0xBD;
static const int PLM_DEMUX_PACKET_AUDIO_1 = 0xC0;
static const int PLM_DEMUX_PACKET_AUDIO_2 = 0xC1;
static const int PLM_DEMUX_PACKET_AUDIO_3 = 0xC2;
static const int PLM_DEMUX_PACKET_AUDIO_4 = 0xC3;
static const int PLM_DEMUX_PACKET_VIDEO_1 = 0xE0;

// Create a demuxer with a plm_buffer as source. This will also attempt to
// read the pack and system headers from the buffer.

plm_demux_t *plm_demux_create(plm_buffer_t *buffer, int destroy_when_done);

// Destroy a demuxer and free all data.

void plm_demux_destroy(plm_demux_t *self);

// Returns TRUE/FALSE whether pack and system headers have been found. This
// will attempt to read the headers if non are present yet.

int plm_demux_has_headers(plm_demux_t *self);

// Probe the file for the actual number of video/audio streams. See
// plm_probe() for the details.

int plm_demux_probe(plm_demux_t *self, size_t probesize);

// Returns the number of video streams found in the system header. This will
// attempt to read the system header if non is present yet.

int plm_demux_get_num_video_streams(plm_demux_t *self);

// Returns the number of audio streams found in the system header. This will
// attempt to read the system header if non is present yet.

int plm_demux_get_num_audio_streams(plm_demux_t *self);

// Rewind the internal buffer. See plm_buffer_rewind().

void plm_demux_rewind(plm_demux_t *self);

// Get whether the file has ended. This will be cleared on seeking or rewind.

int plm_demux_has_ended(plm_demux_t *self);

// Seek to a packet of the specified type with a PTS just before specified
// time. If force_intra is TRUE, only packets containing an intra frame will
// be considered - this only makes sense when the type is
// PLM_DEMUX_PACKET_VIDEO_1. Note that the specified time is considered
// 0-based, regardless of the first PTS in the data source.

plm_packet_t *plm_demux_seek(plm_demux_t *self, double time, int type,
                             int force_intra);

// Get the PTS of the first packet of this type. Returns PLM_PACKET_INVALID_TS
// if not packet of this packet type can be found.

double plm_demux_get_start_time(plm_demux_t *self, int type);

// Get the duration for the specified packet type - i.e. the span between the
// the first PTS and the last PTS in the data source. This only makes sense
// when the underlying data source is a file or fixed memory.

double plm_demux_get_duration(plm_demux_t *self, int type);

// Decode and return the next packet. The returned packet_t is valid until
// the next call to plm_demux_decode() or until the demuxer is destroyed.

plm_packet_t *plm_demux_decode(plm_demux_t *self);

// -----------------------------------------------------------------------------
// plm_video public API
// Decode MPEG1 Video ("mpeg1") data into raw YCrCb frames

// Create a video decoder with a plm_buffer as source.

plm_video_t *plm_video_create_with_buffer(plm_buffer_t *buffer,
                                          int destroy_when_done);

// Destroy a video decoder and free all data.

void plm_video_destroy(plm_video_t *self);

// Get whether a sequence header was found and we can accurately report on
// dimensions and framerate.

int plm_video_has_header(plm_video_t *self);

// Get the framerate in frames per second.

double plm_video_get_framerate(plm_video_t *self);
double plm_video_get_pixel_aspect_ratio(plm_video_t *self);

// Get the display width/height.

int plm_video_get_width(plm_video_t *self);
int plm_video_get_height(plm_video_t *self);

// Set "no delay" mode. When enabled, the decoder assumes that the video does
// *not* contain any B-Frames. This is useful for reducing lag when streaming.
// The default is FALSE.

void plm_video_set_no_delay(plm_video_t *self, int no_delay);

// Get the current internal time in seconds.

double plm_video_get_time(plm_video_t *self);

// Set the current internal time in seconds. This is only useful when you
// manipulate the underlying video buffer and want to enforce a correct
// timestamps.

void plm_video_set_time(plm_video_t *self, double time);

// Rewind the internal buffer. See plm_buffer_rewind().

void plm_video_rewind(plm_video_t *self);

// Get whether the file has ended. This will be cleared on rewind.

int plm_video_has_ended(plm_video_t *self);

// Decode and return one frame of video and advance the internal time by
// 1/framerate seconds. The returned frame_t is valid until the next call of
// plm_video_decode() or until the video decoder is destroyed.

plm_frame_t *plm_video_decode(plm_video_t *self);

// Convert the YCrCb data of a frame into interleaved R G B data. The stride
// specifies the width in bytes of the destination buffer. I.e. the number of
// bytes from one line to the next. The stride must be at least
// (frame->width * bytes_per_pixel). The buffer pointed to by *dest must have
// a size of at least (stride * frame->height). Note that the alpha component
// of the dest buffer is always left untouched.

void plm_frame_to_rgb(plm_frame_t *frame, uint8_t *dest, int stride);
void plm_frame_to_bgr(plm_frame_t *frame, uint8_t *dest, int stride);
void plm_frame_to_rgba(plm_frame_t *frame, uint8_t *dest, int stride);
void plm_frame_to_bgra(plm_frame_t *frame, uint8_t *dest, int stride);
void plm_frame_to_argb(plm_frame_t *frame, uint8_t *dest, int stride);
void plm_frame_to_abgr(plm_frame_t *frame, uint8_t *dest, int stride);

// -----------------------------------------------------------------------------
// plm_audio public API
// Decode MPEG-1 Audio Layer II ("mp2") data into raw samples

// Create an audio decoder with a plm_buffer as source.

plm_audio_t *plm_audio_create_with_buffer(plm_buffer_t *buffer,
                                          int destroy_when_done);

// Destroy an audio decoder and free all data.

void plm_audio_destroy(plm_audio_t *self);

// Get whether a frame header was found and we can accurately report on
// samplerate.

int plm_audio_has_header(plm_audio_t *self);

// Get the samplerate in samples per second.

int plm_audio_get_samplerate(plm_audio_t *self);

// Get the current internal time in seconds.

double plm_audio_get_time(plm_audio_t *self);

// Set the current internal time in seconds. This is only useful when you
// manipulate the underlying video buffer and want to enforce a correct
// timestamps.

void plm_audio_set_time(plm_audio_t *self, double time);

// Rewind the internal buffer. See plm_buffer_rewind().

void plm_audio_rewind(plm_audio_t *self);

// Get whether the file has ended. This will be cleared on rewind.

int plm_audio_has_ended(plm_audio_t *self);

// Decode and return one "frame" of audio and advance the internal time by
// (PLM_AUDIO_SAMPLES_PER_FRAME/samplerate) seconds. The returned samples_t
// is valid until the next call of plm_audio_decode() or until the audio
// decoder is destroyed.

plm_samples_t *plm_audio_decode(plm_audio_t *self);

#ifdef __cplusplus
}
#endif

//#endif  // PL_MPEG_H

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// IMPLEMENTATION

//#include <stdlib.h>
//#include <string.h>

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

#define PLM_UNUSED(expr) (void)(expr)

// -----------------------------------------------------------------------------
// plm (high-level interface) implementation

struct plm_t {
  plm_demux_t *demux;
  double time;
  int has_ended;
  int loop;
  int has_decoders;

  int video_enabled;
  int video_packet_type;
  plm_buffer_t *video_buffer;
  plm_video_t *video_decoder;

  int audio_enabled;
  int audio_stream_index;
  int audio_packet_type;
  double audio_lead_time;
  plm_buffer_t *audio_buffer;
  plm_audio_t *audio_decoder;

  plm_video_decode_callback video_decode_callback;
  void *video_decode_callback_user_data;

  plm_audio_decode_callback audio_decode_callback;
  void *audio_decode_callback_user_data;
};

int plm_init_decoders(plm_t *self);
void plm_handle_end(plm_t *self);
void plm_read_video_packet(plm_buffer_t *buffer, void *user);
void plm_read_audio_packet(plm_buffer_t *buffer, void *user);
void plm_read_packets(plm_t *self, int requested_type);
// -----------------------------------------------------------------------------
// plm_buffer implementation

enum plm_buffer_mode {
  PLM_BUFFER_MODE_FILE,
  PLM_BUFFER_MODE_FIXED_MEM,
  PLM_BUFFER_MODE_RING,
  PLM_BUFFER_MODE_APPEND
};

struct plm_buffer_t {
  size_t bit_index;
  size_t capacity;
  size_t length;
  size_t total_size;
  int discard_read_bytes;
  int has_ended;
  int free_when_done;
  int close_when_done;
  FILE *fh;
  plm_buffer_load_callback load_callback;
  void *load_callback_user_data;
  alignas(128) uint8_t bytes[PLM_BUFFER_DEFAULT_SIZE];
  enum plm_buffer_mode mode;
};

typedef struct {
  int16_t index;
  int16_t value;
} plm_vlc_t;

typedef struct {
  int16_t index;
  uint16_t value;
} plm_vlc_uint_t;

void plm_buffer_seek(plm_buffer_t *self, size_t pos);
size_t plm_buffer_tell(plm_buffer_t *self);
void plm_buffer_discard_read_bytes(plm_buffer_t *self);
void plm_buffer_load_file_callback(plm_buffer_t *self, void *user);

int plm_buffer_has(plm_buffer_t *self, size_t count);
int plm_buffer_read(plm_buffer_t *self, int count);
void plm_buffer_align(plm_buffer_t *self);
void plm_buffer_skip(plm_buffer_t *self, size_t count);
int plm_buffer_skip_bytes(plm_buffer_t *self, uint8_t v);
int plm_buffer_next_start_code(plm_buffer_t *self);
int plm_buffer_find_start_code(plm_buffer_t *self, int code);
int plm_buffer_no_start_code(plm_buffer_t *self);
int16_t plm_buffer_read_vlc(plm_buffer_t *self, const plm_vlc_t *table);
uint16_t plm_buffer_read_vlc_uint(plm_buffer_t *self,
                                  const plm_vlc_uint_t *table);

// ----------------------------------------------------------------------------
// plm_demux implementation

static const int PLM_START_PACK = 0xBA;
static const int PLM_START_END = 0xB9;
static const int PLM_START_SYSTEM = 0xBB;

struct plm_demux_t {
  plm_buffer_t *buffer;
  int destroy_buffer_when_done;
  double system_clock_ref;

  size_t last_file_size;
  double last_decoded_pts;
  double start_time;
  double duration;

  int start_code;
  int has_pack_header;
  int has_system_header;
  int has_headers;

  int num_audio_streams;
  int num_video_streams;
  plm_packet_t current_packet;
  plm_packet_t next_packet;
};

void plm_demux_buffer_seek(plm_demux_t *self, size_t pos);
double plm_demux_decode_time(plm_demux_t *self);
plm_packet_t *plm_demux_decode_packet(plm_demux_t *self, int type);
plm_packet_t *plm_demux_get_packet(plm_demux_t *self);

static plm_demux_t static_demux_holder;

// -----------------------------------------------------------------------------
// plm_video implementation

// Inspired by Java MPEG-1 Video Decoder and Player by Zoltan Korandi
// https://sourceforge.net/projects/javampeg1video/

static const int PLM_VIDEO_PICTURE_TYPE_INTRA = 1;
static const int PLM_VIDEO_PICTURE_TYPE_PREDICTIVE = 2;
static const int PLM_VIDEO_PICTURE_TYPE_B = 3;

static const int PLM_START_SEQUENCE = 0xB3;
static const int PLM_START_SLICE_FIRST = 0x01;
static const int PLM_START_SLICE_LAST = 0xAF;
static const int PLM_START_PICTURE = 0x00;
static const int PLM_START_EXTENSION = 0xB5;
static const int PLM_START_USER_DATA = 0xB2;

#define PLM_START_IS_SLICE(c) \
  (c >= PLM_START_SLICE_FIRST && c <= PLM_START_SLICE_LAST)

static const float PLM_VIDEO_PIXEL_ASPECT_RATIO[] = {
    1.0000, /* square pixels */
    0.6735, /* 3:4? */
    0.7031, /* MPEG-1 / MPEG-2 video encoding divergence? */
    0.7615, 0.8055, 0.8437, 0.8935, 0.9157, 0.9815,
    1.0255, 1.0695, 1.0950, 1.1575, 1.2051,
};

static const double PLM_VIDEO_PICTURE_RATE[] = {
    0.000,  23.976, 24.000, 25.000, 29.970, 30.000, 50.000, 59.940,
    60.000, 0.000,  0.000,  0.000,  0.000,  0.000,  0.000,  0.000};

static const uint8_t PLM_VIDEO_ZIG_ZAG[] = {
    0,  1,  8,  16, 9,  2,  3,  10, 17, 24, 32, 25, 18, 11, 4,  5,
    12, 19, 26, 33, 40, 48, 41, 34, 27, 20, 13, 6,  7,  14, 21, 28,
    35, 42, 49, 56, 57, 50, 43, 36, 29, 22, 15, 23, 30, 37, 44, 51,
    58, 59, 52, 45, 38, 31, 39, 46, 53, 60, 61, 54, 47, 55, 62, 63};

alignas(128) static const uint8_t PLM_VIDEO_INTRA_QUANT_MATRIX[] = {
    8,  16, 19, 22, 26, 27, 29, 34, 16, 16, 22, 24, 27, 29, 34, 37,
    19, 22, 26, 27, 29, 34, 34, 38, 22, 22, 26, 27, 29, 34, 37, 40,
    22, 26, 27, 29, 32, 35, 40, 48, 26, 27, 29, 32, 35, 40, 48, 58,
    26, 27, 29, 34, 38, 46, 56, 69, 27, 29, 35, 38, 46, 56, 69, 83};

alignas(128) static const uint8_t PLM_VIDEO_NON_INTRA_QUANT_MATRIX[] = {
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16};

static const uint8_t PLM_VIDEO_PREMULTIPLIER_MATRIX[] = {
    32, 44, 42, 38, 32, 25, 17, 9,  44, 62, 58, 52, 44, 35, 24, 12,
    42, 58, 55, 49, 42, 33, 23, 12, 38, 52, 49, 44, 38, 30, 20, 10,
    32, 44, 42, 38, 32, 25, 17, 9,  25, 35, 33, 30, 25, 20, 14, 7,
    17, 24, 23, 20, 17, 14, 9,  5,  9,  12, 12, 10, 9,  7,  5,  2};

static const plm_vlc_t PLM_VIDEO_MACROBLOCK_ADDRESS_INCREMENT[] = {
    {1 << 1, 0},  {0, 1},        //   0: x
    {2 << 1, 0},  {3 << 1, 0},   //   1: 0x
    {4 << 1, 0},  {5 << 1, 0},   //   2: 00x
    {0, 3},       {0, 2},        //   3: 01x
    {6 << 1, 0},  {7 << 1, 0},   //   4: 000x
    {0, 5},       {0, 4},        //   5: 001x
    {8 << 1, 0},  {9 << 1, 0},   //   6: 0000x
    {0, 7},       {0, 6},        //   7: 0001x
    {10 << 1, 0}, {11 << 1, 0},  //   8: 0000 0x
    {12 << 1, 0}, {13 << 1, 0},  //   9: 0000 1x
    {14 << 1, 0}, {15 << 1, 0},  //  10: 0000 00x
    {16 << 1, 0}, {17 << 1, 0},  //  11: 0000 01x
    {18 << 1, 0}, {19 << 1, 0},  //  12: 0000 10x
    {0, 9},       {0, 8},        //  13: 0000 11x
    {-1, 0},      {20 << 1, 0},  //  14: 0000 000x
    {-1, 0},      {21 << 1, 0},  //  15: 0000 001x
    {22 << 1, 0}, {23 << 1, 0},  //  16: 0000 010x
    {0, 15},      {0, 14},       //  17: 0000 011x
    {0, 13},      {0, 12},       //  18: 0000 100x
    {0, 11},      {0, 10},       //  19: 0000 101x
    {24 << 1, 0}, {25 << 1, 0},  //  20: 0000 0001x
    {26 << 1, 0}, {27 << 1, 0},  //  21: 0000 0011x
    {28 << 1, 0}, {29 << 1, 0},  //  22: 0000 0100x
    {30 << 1, 0}, {31 << 1, 0},  //  23: 0000 0101x
    {32 << 1, 0}, {-1, 0},       //  24: 0000 0001 0x
    {-1, 0},      {33 << 1, 0},  //  25: 0000 0001 1x
    {34 << 1, 0}, {35 << 1, 0},  //  26: 0000 0011 0x
    {36 << 1, 0}, {37 << 1, 0},  //  27: 0000 0011 1x
    {38 << 1, 0}, {39 << 1, 0},  //  28: 0000 0100 0x
    {0, 21},      {0, 20},       //  29: 0000 0100 1x
    {0, 19},      {0, 18},       //  30: 0000 0101 0x
    {0, 17},      {0, 16},       //  31: 0000 0101 1x
    {0, 35},      {-1, 0},       //  32: 0000 0001 00x
    {-1, 0},      {0, 34},       //  33: 0000 0001 11x
    {0, 33},      {0, 32},       //  34: 0000 0011 00x
    {0, 31},      {0, 30},       //  35: 0000 0011 01x
    {0, 29},      {0, 28},       //  36: 0000 0011 10x
    {0, 27},      {0, 26},       //  37: 0000 0011 11x
    {0, 25},      {0, 24},       //  38: 0000 0100 00x
    {0, 23},      {0, 22},       //  39: 0000 0100 01x
};

static const plm_vlc_t PLM_VIDEO_MACROBLOCK_TYPE_INTRA[] = {
    {1 << 1, 0},
    {0, 0x01},  //   0: x
    {-1, 0},
    {0, 0x11},  //   1: 0x
};

static const plm_vlc_t PLM_VIDEO_MACROBLOCK_TYPE_PREDICTIVE[] = {
    {1 << 1, 0}, {0, 0x0a},    //   0: x
    {2 << 1, 0}, {0, 0x02},    //   1: 0x
    {3 << 1, 0}, {0, 0x08},    //   2: 00x
    {4 << 1, 0}, {5 << 1, 0},  //   3: 000x
    {6 << 1, 0}, {0, 0x12},    //   4: 0000x
    {0, 0x1a},   {0, 0x01},    //   5: 0001x
    {-1, 0},     {0, 0x11},    //   6: 0000 0x
};

static const plm_vlc_t PLM_VIDEO_MACROBLOCK_TYPE_B[] = {
    {1 << 1, 0}, {2 << 1, 0},   //   0: x
    {3 << 1, 0}, {4 << 1, 0},   //   1: 0x
    {0, 0x0c},   {0, 0x0e},     //   2: 1x
    {5 << 1, 0}, {6 << 1, 0},   //   3: 00x
    {0, 0x04},   {0, 0x06},     //   4: 01x
    {7 << 1, 0}, {8 << 1, 0},   //   5: 000x
    {0, 0x08},   {0, 0x0a},     //   6: 001x
    {9 << 1, 0}, {10 << 1, 0},  //   7: 0000x
    {0, 0x1e},   {0, 0x01},     //   8: 0001x
    {-1, 0},     {0, 0x11},     //   9: 0000 0x
    {0, 0x16},   {0, 0x1a},     //  10: 0000 1x
};

static const plm_vlc_t *PLM_VIDEO_MACROBLOCK_TYPE[] = {
    NULL, PLM_VIDEO_MACROBLOCK_TYPE_INTRA, PLM_VIDEO_MACROBLOCK_TYPE_PREDICTIVE,
    PLM_VIDEO_MACROBLOCK_TYPE_B};

static const plm_vlc_t PLM_VIDEO_CODE_BLOCK_PATTERN[] = {
    {1 << 1, 0},  {2 << 1, 0},   //   0: x
    {3 << 1, 0},  {4 << 1, 0},   //   1: 0x
    {5 << 1, 0},  {6 << 1, 0},   //   2: 1x
    {7 << 1, 0},  {8 << 1, 0},   //   3: 00x
    {9 << 1, 0},  {10 << 1, 0},  //   4: 01x
    {11 << 1, 0}, {12 << 1, 0},  //   5: 10x
    {13 << 1, 0}, {0, 60},       //   6: 11x
    {14 << 1, 0}, {15 << 1, 0},  //   7: 000x
    {16 << 1, 0}, {17 << 1, 0},  //   8: 001x
    {18 << 1, 0}, {19 << 1, 0},  //   9: 010x
    {20 << 1, 0}, {21 << 1, 0},  //  10: 011x
    {22 << 1, 0}, {23 << 1, 0},  //  11: 100x
    {0, 32},      {0, 16},       //  12: 101x
    {0, 8},       {0, 4},        //  13: 110x
    {24 << 1, 0}, {25 << 1, 0},  //  14: 0000x
    {26 << 1, 0}, {27 << 1, 0},  //  15: 0001x
    {28 << 1, 0}, {29 << 1, 0},  //  16: 0010x
    {30 << 1, 0}, {31 << 1, 0},  //  17: 0011x
    {0, 62},      {0, 2},        //  18: 0100x
    {0, 61},      {0, 1},        //  19: 0101x
    {0, 56},      {0, 52},       //  20: 0110x
    {0, 44},      {0, 28},       //  21: 0111x
    {0, 40},      {0, 20},       //  22: 1000x
    {0, 48},      {0, 12},       //  23: 1001x
    {32 << 1, 0}, {33 << 1, 0},  //  24: 0000 0x
    {34 << 1, 0}, {35 << 1, 0},  //  25: 0000 1x
    {36 << 1, 0}, {37 << 1, 0},  //  26: 0001 0x
    {38 << 1, 0}, {39 << 1, 0},  //  27: 0001 1x
    {40 << 1, 0}, {41 << 1, 0},  //  28: 0010 0x
    {42 << 1, 0}, {43 << 1, 0},  //  29: 0010 1x
    {0, 63},      {0, 3},        //  30: 0011 0x
    {0, 36},      {0, 24},       //  31: 0011 1x
    {44 << 1, 0}, {45 << 1, 0},  //  32: 0000 00x
    {46 << 1, 0}, {47 << 1, 0},  //  33: 0000 01x
    {48 << 1, 0}, {49 << 1, 0},  //  34: 0000 10x
    {50 << 1, 0}, {51 << 1, 0},  //  35: 0000 11x
    {52 << 1, 0}, {53 << 1, 0},  //  36: 0001 00x
    {54 << 1, 0}, {55 << 1, 0},  //  37: 0001 01x
    {56 << 1, 0}, {57 << 1, 0},  //  38: 0001 10x
    {58 << 1, 0}, {59 << 1, 0},  //  39: 0001 11x
    {0, 34},      {0, 18},       //  40: 0010 00x
    {0, 10},      {0, 6},        //  41: 0010 01x
    {0, 33},      {0, 17},       //  42: 0010 10x
    {0, 9},       {0, 5},        //  43: 0010 11x
    {-1, 0},      {60 << 1, 0},  //  44: 0000 000x
    {61 << 1, 0}, {62 << 1, 0},  //  45: 0000 001x
    {0, 58},      {0, 54},       //  46: 0000 010x
    {0, 46},      {0, 30},       //  47: 0000 011x
    {0, 57},      {0, 53},       //  48: 0000 100x
    {0, 45},      {0, 29},       //  49: 0000 101x
    {0, 38},      {0, 26},       //  50: 0000 110x
    {0, 37},      {0, 25},       //  51: 0000 111x
    {0, 43},      {0, 23},       //  52: 0001 000x
    {0, 51},      {0, 15},       //  53: 0001 001x
    {0, 42},      {0, 22},       //  54: 0001 010x
    {0, 50},      {0, 14},       //  55: 0001 011x
    {0, 41},      {0, 21},       //  56: 0001 100x
    {0, 49},      {0, 13},       //  57: 0001 101x
    {0, 35},      {0, 19},       //  58: 0001 110x
    {0, 11},      {0, 7},        //  59: 0001 111x
    {0, 39},      {0, 27},       //  60: 0000 0001x
    {0, 59},      {0, 55},       //  61: 0000 0010x
    {0, 47},      {0, 31},       //  62: 0000 0011x
};

static const plm_vlc_t PLM_VIDEO_MOTION[] = {
    {1 << 1, 0},  {0, 0},        //   0: x
    {2 << 1, 0},  {3 << 1, 0},   //   1: 0x
    {4 << 1, 0},  {5 << 1, 0},   //   2: 00x
    {0, 1},       {0, -1},       //   3: 01x
    {6 << 1, 0},  {7 << 1, 0},   //   4: 000x
    {0, 2},       {0, -2},       //   5: 001x
    {8 << 1, 0},  {9 << 1, 0},   //   6: 0000x
    {0, 3},       {0, -3},       //   7: 0001x
    {10 << 1, 0}, {11 << 1, 0},  //   8: 0000 0x
    {12 << 1, 0}, {13 << 1, 0},  //   9: 0000 1x
    {-1, 0},      {14 << 1, 0},  //  10: 0000 00x
    {15 << 1, 0}, {16 << 1, 0},  //  11: 0000 01x
    {17 << 1, 0}, {18 << 1, 0},  //  12: 0000 10x
    {0, 4},       {0, -4},       //  13: 0000 11x
    {-1, 0},      {19 << 1, 0},  //  14: 0000 001x
    {20 << 1, 0}, {21 << 1, 0},  //  15: 0000 010x
    {0, 7},       {0, -7},       //  16: 0000 011x
    {0, 6},       {0, -6},       //  17: 0000 100x
    {0, 5},       {0, -5},       //  18: 0000 101x
    {22 << 1, 0}, {23 << 1, 0},  //  19: 0000 0011x
    {24 << 1, 0}, {25 << 1, 0},  //  20: 0000 0100x
    {26 << 1, 0}, {27 << 1, 0},  //  21: 0000 0101x
    {28 << 1, 0}, {29 << 1, 0},  //  22: 0000 0011 0x
    {30 << 1, 0}, {31 << 1, 0},  //  23: 0000 0011 1x
    {32 << 1, 0}, {33 << 1, 0},  //  24: 0000 0100 0x
    {0, 10},      {0, -10},      //  25: 0000 0100 1x
    {0, 9},       {0, -9},       //  26: 0000 0101 0x
    {0, 8},       {0, -8},       //  27: 0000 0101 1x
    {0, 16},      {0, -16},      //  28: 0000 0011 00x
    {0, 15},      {0, -15},      //  29: 0000 0011 01x
    {0, 14},      {0, -14},      //  30: 0000 0011 10x
    {0, 13},      {0, -13},      //  31: 0000 0011 11x
    {0, 12},      {0, -12},      //  32: 0000 0100 00x
    {0, 11},      {0, -11},      //  33: 0000 0100 01x
};

static const plm_vlc_t PLM_VIDEO_DCT_SIZE_LUMINANCE[] = {
    {1 << 1, 0}, {2 << 1, 0},  //   0: x
    {0, 1},      {0, 2},       //   1: 0x
    {3 << 1, 0}, {4 << 1, 0},  //   2: 1x
    {0, 0},      {0, 3},       //   3: 10x
    {0, 4},      {5 << 1, 0},  //   4: 11x
    {0, 5},      {6 << 1, 0},  //   5: 111x
    {0, 6},      {7 << 1, 0},  //   6: 1111x
    {0, 7},      {8 << 1, 0},  //   7: 1111 1x
    {0, 8},      {-1, 0},      //   8: 1111 11x
};

static const plm_vlc_t PLM_VIDEO_DCT_SIZE_CHROMINANCE[] = {
    {1 << 1, 0}, {2 << 1, 0},  //   0: x
    {0, 0},      {0, 1},       //   1: 0x
    {0, 2},      {3 << 1, 0},  //   2: 1x
    {0, 3},      {4 << 1, 0},  //   3: 11x
    {0, 4},      {5 << 1, 0},  //   4: 111x
    {0, 5},      {6 << 1, 0},  //   5: 1111x
    {0, 6},      {7 << 1, 0},  //   6: 1111 1x
    {0, 7},      {8 << 1, 0},  //   7: 1111 11x
    {0, 8},      {-1, 0},      //   8: 1111 111x
};

static const plm_vlc_t *PLM_VIDEO_DCT_SIZE[] = {PLM_VIDEO_DCT_SIZE_LUMINANCE,
                                                PLM_VIDEO_DCT_SIZE_CHROMINANCE,
                                                PLM_VIDEO_DCT_SIZE_CHROMINANCE};

//  dct_coeff bitmap:
//    0xff00  run
//    0x00ff  level

//  Decoded values are unsigned. Sign bit follows in the stream.

static const plm_vlc_uint_t PLM_VIDEO_DCT_COEFF[] = {
    {1 << 1, 0},   {0, 0x0001},    //   0: x
    {2 << 1, 0},   {3 << 1, 0},    //   1: 0x
    {4 << 1, 0},   {5 << 1, 0},    //   2: 00x
    {6 << 1, 0},   {0, 0x0101},    //   3: 01x
    {7 << 1, 0},   {8 << 1, 0},    //   4: 000x
    {9 << 1, 0},   {10 << 1, 0},   //   5: 001x
    {0, 0x0002},   {0, 0x0201},    //   6: 010x
    {11 << 1, 0},  {12 << 1, 0},   //   7: 0000x
    {13 << 1, 0},  {14 << 1, 0},   //   8: 0001x
    {15 << 1, 0},  {0, 0x0003},    //   9: 0010x
    {0, 0x0401},   {0, 0x0301},    //  10: 0011x
    {16 << 1, 0},  {0, 0xffff},    //  11: 0000 0x
    {17 << 1, 0},  {18 << 1, 0},   //  12: 0000 1x
    {0, 0x0701},   {0, 0x0601},    //  13: 0001 0x
    {0, 0x0102},   {0, 0x0501},    //  14: 0001 1x
    {19 << 1, 0},  {20 << 1, 0},   //  15: 0010 0x
    {21 << 1, 0},  {22 << 1, 0},   //  16: 0000 00x
    {0, 0x0202},   {0, 0x0901},    //  17: 0000 10x
    {0, 0x0004},   {0, 0x0801},    //  18: 0000 11x
    {23 << 1, 0},  {24 << 1, 0},   //  19: 0010 00x
    {25 << 1, 0},  {26 << 1, 0},   //  20: 0010 01x
    {27 << 1, 0},  {28 << 1, 0},   //  21: 0000 000x
    {29 << 1, 0},  {30 << 1, 0},   //  22: 0000 001x
    {0, 0x0d01},   {0, 0x0006},    //  23: 0010 000x
    {0, 0x0c01},   {0, 0x0b01},    //  24: 0010 001x
    {0, 0x0302},   {0, 0x0103},    //  25: 0010 010x
    {0, 0x0005},   {0, 0x0a01},    //  26: 0010 011x
    {31 << 1, 0},  {32 << 1, 0},   //  27: 0000 0000x
    {33 << 1, 0},  {34 << 1, 0},   //  28: 0000 0001x
    {35 << 1, 0},  {36 << 1, 0},   //  29: 0000 0010x
    {37 << 1, 0},  {38 << 1, 0},   //  30: 0000 0011x
    {39 << 1, 0},  {40 << 1, 0},   //  31: 0000 0000 0x
    {41 << 1, 0},  {42 << 1, 0},   //  32: 0000 0000 1x
    {43 << 1, 0},  {44 << 1, 0},   //  33: 0000 0001 0x
    {45 << 1, 0},  {46 << 1, 0},   //  34: 0000 0001 1x
    {0, 0x1001},   {0, 0x0502},    //  35: 0000 0010 0x
    {0, 0x0007},   {0, 0x0203},    //  36: 0000 0010 1x
    {0, 0x0104},   {0, 0x0f01},    //  37: 0000 0011 0x
    {0, 0x0e01},   {0, 0x0402},    //  38: 0000 0011 1x
    {47 << 1, 0},  {48 << 1, 0},   //  39: 0000 0000 00x
    {49 << 1, 0},  {50 << 1, 0},   //  40: 0000 0000 01x
    {51 << 1, 0},  {52 << 1, 0},   //  41: 0000 0000 10x
    {53 << 1, 0},  {54 << 1, 0},   //  42: 0000 0000 11x
    {55 << 1, 0},  {56 << 1, 0},   //  43: 0000 0001 00x
    {57 << 1, 0},  {58 << 1, 0},   //  44: 0000 0001 01x
    {59 << 1, 0},  {60 << 1, 0},   //  45: 0000 0001 10x
    {61 << 1, 0},  {62 << 1, 0},   //  46: 0000 0001 11x
    {-1, 0},       {63 << 1, 0},   //  47: 0000 0000 000x
    {64 << 1, 0},  {65 << 1, 0},   //  48: 0000 0000 001x
    {66 << 1, 0},  {67 << 1, 0},   //  49: 0000 0000 010x
    {68 << 1, 0},  {69 << 1, 0},   //  50: 0000 0000 011x
    {70 << 1, 0},  {71 << 1, 0},   //  51: 0000 0000 100x
    {72 << 1, 0},  {73 << 1, 0},   //  52: 0000 0000 101x
    {74 << 1, 0},  {75 << 1, 0},   //  53: 0000 0000 110x
    {76 << 1, 0},  {77 << 1, 0},   //  54: 0000 0000 111x
    {0, 0x000b},   {0, 0x0802},    //  55: 0000 0001 000x
    {0, 0x0403},   {0, 0x000a},    //  56: 0000 0001 001x
    {0, 0x0204},   {0, 0x0702},    //  57: 0000 0001 010x
    {0, 0x1501},   {0, 0x1401},    //  58: 0000 0001 011x
    {0, 0x0009},   {0, 0x1301},    //  59: 0000 0001 100x
    {0, 0x1201},   {0, 0x0105},    //  60: 0000 0001 101x
    {0, 0x0303},   {0, 0x0008},    //  61: 0000 0001 110x
    {0, 0x0602},   {0, 0x1101},    //  62: 0000 0001 111x
    {78 << 1, 0},  {79 << 1, 0},   //  63: 0000 0000 0001x
    {80 << 1, 0},  {81 << 1, 0},   //  64: 0000 0000 0010x
    {82 << 1, 0},  {83 << 1, 0},   //  65: 0000 0000 0011x
    {84 << 1, 0},  {85 << 1, 0},   //  66: 0000 0000 0100x
    {86 << 1, 0},  {87 << 1, 0},   //  67: 0000 0000 0101x
    {88 << 1, 0},  {89 << 1, 0},   //  68: 0000 0000 0110x
    {90 << 1, 0},  {91 << 1, 0},   //  69: 0000 0000 0111x
    {0, 0x0a02},   {0, 0x0902},    //  70: 0000 0000 1000x
    {0, 0x0503},   {0, 0x0304},    //  71: 0000 0000 1001x
    {0, 0x0205},   {0, 0x0107},    //  72: 0000 0000 1010x
    {0, 0x0106},   {0, 0x000f},    //  73: 0000 0000 1011x
    {0, 0x000e},   {0, 0x000d},    //  74: 0000 0000 1100x
    {0, 0x000c},   {0, 0x1a01},    //  75: 0000 0000 1101x
    {0, 0x1901},   {0, 0x1801},    //  76: 0000 0000 1110x
    {0, 0x1701},   {0, 0x1601},    //  77: 0000 0000 1111x
    {92 << 1, 0},  {93 << 1, 0},   //  78: 0000 0000 0001 0x
    {94 << 1, 0},  {95 << 1, 0},   //  79: 0000 0000 0001 1x
    {96 << 1, 0},  {97 << 1, 0},   //  80: 0000 0000 0010 0x
    {98 << 1, 0},  {99 << 1, 0},   //  81: 0000 0000 0010 1x
    {100 << 1, 0}, {101 << 1, 0},  //  82: 0000 0000 0011 0x
    {102 << 1, 0}, {103 << 1, 0},  //  83: 0000 0000 0011 1x
    {0, 0x001f},   {0, 0x001e},    //  84: 0000 0000 0100 0x
    {0, 0x001d},   {0, 0x001c},    //  85: 0000 0000 0100 1x
    {0, 0x001b},   {0, 0x001a},    //  86: 0000 0000 0101 0x
    {0, 0x0019},   {0, 0x0018},    //  87: 0000 0000 0101 1x
    {0, 0x0017},   {0, 0x0016},    //  88: 0000 0000 0110 0x
    {0, 0x0015},   {0, 0x0014},    //  89: 0000 0000 0110 1x
    {0, 0x0013},   {0, 0x0012},    //  90: 0000 0000 0111 0x
    {0, 0x0011},   {0, 0x0010},    //  91: 0000 0000 0111 1x
    {104 << 1, 0}, {105 << 1, 0},  //  92: 0000 0000 0001 00x
    {106 << 1, 0}, {107 << 1, 0},  //  93: 0000 0000 0001 01x
    {108 << 1, 0}, {109 << 1, 0},  //  94: 0000 0000 0001 10x
    {110 << 1, 0}, {111 << 1, 0},  //  95: 0000 0000 0001 11x
    {0, 0x0028},   {0, 0x0027},    //  96: 0000 0000 0010 00x
    {0, 0x0026},   {0, 0x0025},    //  97: 0000 0000 0010 01x
    {0, 0x0024},   {0, 0x0023},    //  98: 0000 0000 0010 10x
    {0, 0x0022},   {0, 0x0021},    //  99: 0000 0000 0010 11x
    {0, 0x0020},   {0, 0x010e},    // 100: 0000 0000 0011 00x
    {0, 0x010d},   {0, 0x010c},    // 101: 0000 0000 0011 01x
    {0, 0x010b},   {0, 0x010a},    // 102: 0000 0000 0011 10x
    {0, 0x0109},   {0, 0x0108},    // 103: 0000 0000 0011 11x
    {0, 0x0112},   {0, 0x0111},    // 104: 0000 0000 0001 000x
    {0, 0x0110},   {0, 0x010f},    // 105: 0000 0000 0001 001x
    {0, 0x0603},   {0, 0x1002},    // 106: 0000 0000 0001 010x
    {0, 0x0f02},   {0, 0x0e02},    // 107: 0000 0000 0001 011x
    {0, 0x0d02},   {0, 0x0c02},    // 108: 0000 0000 0001 100x
    {0, 0x0b02},   {0, 0x1f01},    // 109: 0000 0000 0001 101x
    {0, 0x1e01},   {0, 0x1d01},    // 110: 0000 0000 0001 110x
    {0, 0x1c01},   {0, 0x1b01},    // 111: 0000 0000 0001 111x
};

typedef struct {
  int full_px;
  int is_set;
  int r_size;
  int h;
  int v;
} plm_video_motion_t;

struct plm_video_t {
  double framerate;
  double pixel_aspect_ratio;
  double time;
  int frames_decoded;
  int width;
  int height;
  int mb_width;
  int mb_height;
  int mb_size;

  int luma_width;
  int luma_height;

  int chroma_width;
  int chroma_height;

  int start_code;
  int picture_type;

  plm_video_motion_t motion_forward;
  plm_video_motion_t motion_backward;

  int has_sequence_header;

  int quantizer_scale;
  int slice_begin;
  int macroblock_address;

  int mb_row;
  int mb_col;

  int macroblock_type;
  int macroblock_intra;

  int dc_predictor[3];

  plm_buffer_t *buffer;
  int destroy_buffer_when_done;

  plm_frame_t frame_current;
  plm_frame_t frame_forward;
  plm_frame_t frame_backward;

  uint8_t frames_data[4147200];  // expand if needed for video
                                 // 4147200 for intersection.mpg
  int block_data[64];
  uint8_t intra_quant_matrix[64];
  uint8_t non_intra_quant_matrix[64];

  int has_reference_frame;
  int assume_no_b_frames;
};

static inline uint8_t plm_clamp(int n) {
  if (n > 255) {
    n = 255;
  } else if (n < 0) {
    n = 0;
  }
  return n;
}

int plm_video_decode_sequence_header(plm_video_t *self);
void plm_video_init_frame(plm_video_t *self, plm_frame_t *frame, uint8_t *base);
void plm_video_decode_picture(plm_video_t *self);
void plm_video_decode_slice(plm_video_t *self, int slice);
void plm_video_decode_macroblock(plm_video_t *self);
void plm_video_decode_motion_vectors(plm_video_t *self);
int plm_video_decode_motion_vector(plm_video_t *self, int r_size, int motion);
void plm_video_predict_macroblock(plm_video_t *self);
void plm_video_copy_macroblock(plm_video_t *self, plm_frame_t *s, int motion_h,
                               int motion_v);
void plm_video_interpolate_macroblock(plm_video_t *self, plm_frame_t *s,
                                      int motion_h, int motion_v);
void plm_video_process_macroblock(plm_video_t *self, uint8_t *s, uint8_t *d,
                                  int mh, int mb, int bs, int interp);
void plm_video_decode_block(plm_video_t *self, int block);
void plm_video_idct(int *block);

static plm_video_t static_video_holder;

#define PLM_BLOCK_SET(DEST, DEST_INDEX, DEST_WIDTH, SOURCE_INDEX, \
                      SOURCE_WIDTH, BLOCK_SIZE, OP)               \
  do {                                                            \
    int dest_scan = DEST_WIDTH - BLOCK_SIZE;                      \
    int source_scan = SOURCE_WIDTH - BLOCK_SIZE;                  \
    for (int y = 0; y < BLOCK_SIZE; y++) {                        \
      for (int x = 0; x < BLOCK_SIZE; x++) {                      \
        DEST[DEST_INDEX] = OP;                                    \
        SOURCE_INDEX++;                                           \
        DEST_INDEX++;                                             \
      }                                                           \
      SOURCE_INDEX += source_scan;                                \
      DEST_INDEX += dest_scan;                                    \
    }                                                             \
  } while (FALSE)

// YCbCr conversion following the BT.601 standard:
// https://infogalactic.com/info/YCbCr#ITU-R_BT.601_conversion

#define PLM_PUT_PIXEL(RI, GI, BI, Y_OFFSET, DEST_OFFSET)        \
  y = ((frame->y.data[y_index + Y_OFFSET] - 16) * 76309) >> 16; \
  dest[d_index + DEST_OFFSET + RI] = plm_clamp(y + r);          \
  dest[d_index + DEST_OFFSET + GI] = plm_clamp(y - g);          \
  dest[d_index + DEST_OFFSET + BI] = plm_clamp(y + b);

#define PLM_DEFINE_FRAME_CONVERT_FUNCTION(NAME, BYTES_PER_PIXEL, RI, GI, BI) \
  void NAME(plm_frame_t *frame, uint8_t *dest, int stride) {                 \
    int cols = frame->width >> 1;                                            \
    int rows = frame->height >> 1;                                           \
    int yw = frame->y.width;                                                 \
    int cw = frame->cb.width;                                                \
    for (int row = 0; row < rows; row++) {                                   \
      int c_index = row * cw;                                                \
      int y_index = row * 2 * yw;                                            \
      int d_index = row * 2 * stride;                                        \
      for (int col = 0; col < cols; col++) {                                 \
        int y;                                                               \
        int cr = frame->cr.data[c_index] - 128;                              \
        int cb = frame->cb.data[c_index] - 128;                              \
        int r = (cr * 104597) >> 16;                                         \
        int g = (cb * 25674 + cr * 53278) >> 16;                             \
        int b = (cb * 132201) >> 16;                                         \
        PLM_PUT_PIXEL(RI, GI, BI, 0, 0);                                     \
        PLM_PUT_PIXEL(RI, GI, BI, 1, BYTES_PER_PIXEL);                       \
        PLM_PUT_PIXEL(RI, GI, BI, yw, stride);                               \
        PLM_PUT_PIXEL(RI, GI, BI, yw + 1, stride + BYTES_PER_PIXEL);         \
        c_index += 1;                                                        \
        y_index += 2;                                                        \
        d_index += 2 * BYTES_PER_PIXEL;                                      \
      }                                                                      \
    }                                                                        \
  }

// -----------------------------------------------------------------------------
// plm_audio implementation

// Based on kjmp2 by Martin J. Fiedler
// http://keyj.emphy.de/kjmp2/

static const int PLM_AUDIO_FRAME_SYNC = 0x7ff;

static const int PLM_AUDIO_MPEG_2_5 = 0x0;
static const int PLM_AUDIO_MPEG_2 = 0x2;
static const int PLM_AUDIO_MPEG_1 = 0x3;

static const int PLM_AUDIO_LAYER_III = 0x1;
static const int PLM_AUDIO_LAYER_II = 0x2;
static const int PLM_AUDIO_LAYER_I = 0x3;

static const int PLM_AUDIO_MODE_STEREO = 0x0;
static const int PLM_AUDIO_MODE_JOINT_STEREO = 0x1;
static const int PLM_AUDIO_MODE_DUAL_CHANNEL = 0x2;
static const int PLM_AUDIO_MODE_MONO = 0x3;

static const unsigned short PLM_AUDIO_SAMPLE_RATE[] = {
    44100, 48000, 32000, 0,  // MPEG-1
    22050, 24000, 16000, 0   // MPEG-2
};

static const short PLM_AUDIO_BIT_RATE[] = {
    32, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384,  // MPEG-1
    8,  16, 24, 32, 40, 48, 56,  64,  80,  96,  112, 128, 144, 160   // MPEG-2
};

static const int PLM_AUDIO_SCALEFACTOR_BASE[] = {0x02000000, 0x01965FEA,
                                                 0x01428A30};

static const float PLM_AUDIO_SYNTHESIS_WINDOW[] = {
    0.0,      -0.5,     -0.5,     -0.5,     -0.5,     -0.5,     -0.5,
    -1.0,     -1.0,     -1.0,     -1.0,     -1.5,     -1.5,     -2.0,
    -2.0,     -2.5,     -2.5,     -3.0,     -3.5,     -3.5,     -4.0,
    -4.5,     -5.0,     -5.5,     -6.5,     -7.0,     -8.0,     -8.5,
    -9.5,     -10.5,    -12.0,    -13.0,    -14.5,    -15.5,    -17.5,
    -19.0,    -20.5,    -22.5,    -24.5,    -26.5,    -29.0,    -31.5,
    -34.0,    -36.5,    -39.5,    -42.5,    -45.5,    -48.5,    -52.0,
    -55.5,    -58.5,    -62.5,    -66.0,    -69.5,    -73.5,    -77.0,
    -80.5,    -84.5,    -88.0,    -91.5,    -95.0,    -98.0,    -101.0,
    -104.0,   106.5,    109.0,    111.0,    112.5,    113.5,    114.0,
    114.0,    113.5,    112.0,    110.5,    107.5,    104.0,    100.0,
    94.5,     88.5,     81.5,     73.0,     63.5,     53.0,     41.5,
    28.5,     14.5,     -1.0,     -18.0,    -36.0,    -55.5,    -76.5,
    -98.5,    -122.0,   -147.0,   -173.5,   -200.5,   -229.5,   -259.5,
    -290.5,   -322.5,   -355.5,   -389.5,   -424.0,   -459.5,   -495.5,
    -532.0,   -568.5,   -605.0,   -641.5,   -678.0,   -714.0,   -749.0,
    -783.5,   -817.0,   -849.0,   -879.5,   -908.5,   -935.0,   -959.5,
    -981.0,   -1000.5,  -1016.0,  -1028.5,  -1037.5,  -1042.5,  -1043.5,
    -1040.0,  -1031.5,  1018.5,   1000.0,   976.0,    946.5,    911.0,
    869.5,    822.0,    767.5,    707.0,    640.0,    565.5,    485.0,
    397.0,    302.5,    201.0,    92.5,     -22.5,    -144.0,   -272.5,
    -407.0,   -547.5,   -694.0,   -846.0,   -1003.0,  -1165.0,  -1331.5,
    -1502.0,  -1675.5,  -1852.5,  -2031.5,  -2212.5,  -2394.0,  -2576.5,
    -2758.5,  -2939.5,  -3118.5,  -3294.5,  -3467.5,  -3635.5,  -3798.5,
    -3955.0,  -4104.5,  -4245.5,  -4377.5,  -4499.0,  -4609.5,  -4708.0,
    -4792.5,  -4863.5,  -4919.0,  -4958.0,  -4979.5,  -4983.0,  -4967.5,
    -4931.5,  -4875.0,  -4796.0,  -4694.5,  -4569.5,  -4420.0,  -4246.0,
    -4046.0,  -3820.0,  -3567.0,  3287.0,   2979.5,   2644.0,   2280.5,
    1888.0,   1467.5,   1018.5,   541.0,    35.0,     -499.0,   -1061.0,
    -1650.0,  -2266.5,  -2909.0,  -3577.0,  -4270.0,  -4987.5,  -5727.5,
    -6490.0,  -7274.0,  -8077.5,  -8899.5,  -9739.0,  -10594.5, -11464.5,
    -12347.0, -13241.0, -14144.5, -15056.0, -15973.5, -16895.5, -17820.0,
    -18744.5, -19668.0, -20588.0, -21503.0, -22410.5, -23308.5, -24195.0,
    -25068.5, -25926.5, -26767.0, -27589.0, -28389.0, -29166.5, -29919.0,
    -30644.5, -31342.0, -32009.5, -32645.0, -33247.0, -33814.5, -34346.0,
    -34839.5, -35295.0, -35710.0, -36084.5, -36417.5, -36707.5, -36954.0,
    -37156.5, -37315.0, -37428.0, -37496.0, 37519.0,  37496.0,  37428.0,
    37315.0,  37156.5,  36954.0,  36707.5,  36417.5,  36084.5,  35710.0,
    35295.0,  34839.5,  34346.0,  33814.5,  33247.0,  32645.0,  32009.5,
    31342.0,  30644.5,  29919.0,  29166.5,  28389.0,  27589.0,  26767.0,
    25926.5,  25068.5,  24195.0,  23308.5,  22410.5,  21503.0,  20588.0,
    19668.0,  18744.5,  17820.0,  16895.5,  15973.5,  15056.0,  14144.5,
    13241.0,  12347.0,  11464.5,  10594.5,  9739.0,   8899.5,   8077.5,
    7274.0,   6490.0,   5727.5,   4987.5,   4270.0,   3577.0,   2909.0,
    2266.5,   1650.0,   1061.0,   499.0,    -35.0,    -541.0,   -1018.5,
    -1467.5,  -1888.0,  -2280.5,  -2644.0,  -2979.5,  3287.0,   3567.0,
    3820.0,   4046.0,   4246.0,   4420.0,   4569.5,   4694.5,   4796.0,
    4875.0,   4931.5,   4967.5,   4983.0,   4979.5,   4958.0,   4919.0,
    4863.5,   4792.5,   4708.0,   4609.5,   4499.0,   4377.5,   4245.5,
    4104.5,   3955.0,   3798.5,   3635.5,   3467.5,   3294.5,   3118.5,
    2939.5,   2758.5,   2576.5,   2394.0,   2212.5,   2031.5,   1852.5,
    1675.5,   1502.0,   1331.5,   1165.0,   1003.0,   846.0,    694.0,
    547.5,    407.0,    272.5,    144.0,    22.5,     -92.5,    -201.0,
    -302.5,   -397.0,   -485.0,   -565.5,   -640.0,   -707.0,   -767.5,
    -822.0,   -869.5,   -911.0,   -946.5,   -976.0,   -1000.0,  1018.5,
    1031.5,   1040.0,   1043.5,   1042.5,   1037.5,   1028.5,   1016.0,
    1000.5,   981.0,    959.5,    935.0,    908.5,    879.5,    849.0,
    817.0,    783.5,    749.0,    714.0,    678.0,    641.5,    605.0,
    568.5,    532.0,    495.5,    459.5,    424.0,    389.5,    355.5,
    322.5,    290.5,    259.5,    229.5,    200.5,    173.5,    147.0,
    122.0,    98.5,     76.5,     55.5,     36.0,     18.0,     1.0,
    -14.5,    -28.5,    -41.5,    -53.0,    -63.5,    -73.0,    -81.5,
    -88.5,    -94.5,    -100.0,   -104.0,   -107.5,   -110.5,   -112.0,
    -113.5,   -114.0,   -114.0,   -113.5,   -112.5,   -111.0,   -109.0,
    106.5,    104.0,    101.0,    98.0,     95.0,     91.5,     88.0,
    84.5,     80.5,     77.0,     73.5,     69.5,     66.0,     62.5,
    58.5,     55.5,     52.0,     48.5,     45.5,     42.5,     39.5,
    36.5,     34.0,     31.5,     29.0,     26.5,     24.5,     22.5,
    20.5,     19.0,     17.5,     15.5,     14.5,     13.0,     12.0,
    10.5,     9.5,      8.5,      8.0,      7.0,      6.5,      5.5,
    5.0,      4.5,      4.0,      3.5,      3.5,      3.0,      2.5,
    2.5,      2.0,      2.0,      1.5,      1.5,      1.0,      1.0,
    1.0,      1.0,      0.5,      0.5,      0.5,      0.5,      0.5,
    0.5};

// Quantizer lookup, step 1: bitrate classes
static const uint8_t PLM_AUDIO_QUANT_LUT_STEP_1[2][16] = {
    // 32, 48, 56, 64, 80, 96,112,128,160,192,224,256,320,384 <- bitrate
    {0, 0, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2},  // mono
    // 16, 24, 28, 32, 40, 48, 56, 64, 80, 96,112,128,160,192 <- bitrate / chan
    {0, 0, 0, 0, 0, 0, 1, 1, 1, 2, 2, 2, 2, 2}  // stereo
};

// Quantizer lookup, step 2: bitrate class, sample rate -> B2 table idx, sblimit
#define PLM_AUDIO_QUANT_TAB_A \
  (27 | 64)  // Table 3-B.2a: high-rate, sblimit = 27
#define PLM_AUDIO_QUANT_TAB_B \
  (30 | 64)                       // Table 3-B.2b: high-rate, sblimit = 30
#define PLM_AUDIO_QUANT_TAB_C 8   // Table 3-B.2c:  low-rate, sblimit =  8
#define PLM_AUDIO_QUANT_TAB_D 12  // Table 3-B.2d:  low-rate, sblimit = 12

static const uint8_t QUANT_LUT_STEP_2[3][3] = {
    // 44.1 kHz,              48 kHz,                32 kHz
    {PLM_AUDIO_QUANT_TAB_C, PLM_AUDIO_QUANT_TAB_C,
     PLM_AUDIO_QUANT_TAB_D},  // 32 - 48 kbit/sec/ch
    {PLM_AUDIO_QUANT_TAB_A, PLM_AUDIO_QUANT_TAB_A,
     PLM_AUDIO_QUANT_TAB_A},  // 56 - 80 kbit/sec/ch
    {PLM_AUDIO_QUANT_TAB_B, PLM_AUDIO_QUANT_TAB_A,
     PLM_AUDIO_QUANT_TAB_B}  // 96+	 kbit/sec/ch
};

// Quantizer lookup, step 3: B2 table, subband -> nbal, row index
// (upper 4 bits: nbal, lower 4 bits: row index)
static const uint8_t PLM_AUDIO_QUANT_LUT_STEP_3[3][32] = {
    // Low-rate table (3-B.2c and 3-B.2d)
    {0x44, 0x44, 0x34, 0x34, 0x34, 0x34, 0x34, 0x34, 0x34, 0x34, 0x34, 0x34},
    // High-rate table (3-B.2a and 3-B.2b)
    {0x43, 0x43, 0x43, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42,
     0x42, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31,
     0x31, 0x31, 0x31, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20},
    // MPEG-2 LSR table (B.2 in ISO 13818-3)
    {0x45, 0x45, 0x45, 0x45, 0x34, 0x34, 0x34, 0x34, 0x34, 0x34,
     0x34, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24,
     0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24}};

// Quantizer lookup, step 4: table row, allocation[] value -> quant table index
static const uint8_t PLM_AUDIO_QUANT_LUT_STEP_4[6][16] = {
    {0, 1, 2, 17},
    {0, 1, 2, 3, 4, 5, 6, 17},
    {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 17},
    {0, 1, 3, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17},
    {0, 1, 2, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 17},
    {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15}};

typedef struct plm_quantizer_spec_t {
  unsigned short levels;
  unsigned char group;
  unsigned char bits;
} plm_quantizer_spec_t;

static const plm_quantizer_spec_t PLM_AUDIO_QUANT_TAB[] = {
    {3, 1, 5},       //  1
    {5, 1, 7},       //  2
    {7, 0, 3},       //  3
    {9, 1, 10},      //  4
    {15, 0, 4},      //  5
    {31, 0, 5},      //  6
    {63, 0, 6},      //  7
    {127, 0, 7},     //  8
    {255, 0, 8},     //  9
    {511, 0, 9},     // 10
    {1023, 0, 10},   // 11
    {2047, 0, 11},   // 12
    {4095, 0, 12},   // 13
    {8191, 0, 13},   // 14
    {16383, 0, 14},  // 15
    {32767, 0, 15},  // 16
    {65535, 0, 16}   // 17
};

struct plm_audio_t {
  double time;
  int samples_decoded;
  int samplerate_index;
  int bitrate_index;
  int version;
  int layer;
  int mode;
  int bound;
  int v_pos;
  int next_frame_data_size;
  int has_header;

  plm_buffer_t *buffer;
  int destroy_buffer_when_done;

  const plm_quantizer_spec_t *allocation[2][32];
  uint8_t scale_factor_info[2][32];
  int scale_factor[2][32][3];
  int sample[2][32][3];

  plm_samples_t samples;
  float D[1024];
  float V[2][1024];
  float U[32];
};

int plm_audio_find_frame_sync(plm_audio_t *self);
int plm_audio_decode_header(plm_audio_t *self);
void plm_audio_decode_frame(plm_audio_t *self);
const plm_quantizer_spec_t *plm_audio_read_allocation(plm_audio_t *self, int sb,
                                                      int tab3);
void plm_audio_read_samples(plm_audio_t *self, int ch, int sb, int part);
void plm_audio_idct36(int s[32][3], int ss, float *d, int dp);

static plm_audio_t static_audio_holder;

static plm_buffer_t static_buffer_w_memory_holder;

static plm_buffer_t static_buffer_holder[3];
static int buffer_n = 0;