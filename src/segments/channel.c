#include "internal.h"

struct channel_segment_data{
  struct mixed_channel *channel;
  struct mixed_buffer **buffers;
};

int channel_segment_free(struct mixed_segment *segment){
  if(segment->data){
    free(((struct channel_segment_data *)segment->data)->buffers);
    free(segment->data);
  }
  segment->data = 0;
  return 1;
}

int channel_segment_set_buffer(size_t location, struct mixed_buffer *buffer, struct mixed_segment *segment){
  struct channel_segment_data *data = (struct channel_segment_data *)segment->data;
  if(location<data->channel->channels){
    data->buffers[location] = buffer;
    return 1;
  }else{
    mixed_err(MIXED_INVALID_BUFFER_LOCATION);
    return 0;
  }
}

int source_segment_mix(size_t samples, struct mixed_segment *segment){
  struct channel_segment_data *data = (struct channel_segment_data *)segment->data;
  return mixed_buffer_from_channel(data->channel, data->buffers, samples);
}

int drain_segment_mix(size_t samples, struct mixed_segment *segment){
  struct channel_segment_data *data = (struct channel_segment_data *)segment->data;
  return mixed_buffer_to_channel(data->buffers, data->channel, samples);
}

struct mixed_segment_info source_segment_info(struct mixed_segment *segment){
  struct mixed_segment_info info = {0};
  info.name = "source";
  info.description = "Segment acting as an audio source.";
  info.min_inputs = 0;
  info.max_inputs = 0;
  info.outputs = ((struct channel_segment_data *)segment->data)->channel->channels;
  return info;
}

struct mixed_segment_info drain_segment_info(struct mixed_segment *segment){
  struct mixed_segment_info info = {0};
  info.name = "drain";
  info.description = "Segment acting as an audio drain.";
  info.min_inputs = ((struct channel_segment_data *)segment->data)->channel->channels;
  info.max_inputs = info.min_inputs;
  info.outputs = 0;
  return info;
}

int mixed_make_segment_source(struct mixed_channel *channel, struct mixed_segment *segment){
  struct channel_segment_data *data = calloc(1, sizeof(struct channel_segment_data));
  if(!data){
    mixed_err(MIXED_OUT_OF_MEMORY);
    return 0;
  }

  struct mixed_buffer **buffers = calloc(channel->channels, sizeof(struct mixed_buffer));
  if(!buffers){
    free(data);
    return 0;
  }

  data->buffers = buffers;
  data->channel = channel;

  segment->free = channel_segment_free;
  segment->mix = source_segment_mix;
  segment->set_out = channel_segment_set_buffer;
  segment->info = source_segment_info;
  segment->data = data;
  return 1;
}

int mixed_make_segment_drain(struct mixed_channel *channel, struct mixed_segment *segment){
  struct channel_segment_data *data = calloc(1, sizeof(struct channel_segment_data));
  if(!data){
    mixed_err(MIXED_OUT_OF_MEMORY);
    return 0;
  }

  struct mixed_buffer **buffers = calloc(channel->channels, sizeof(struct mixed_buffer));
  if(!buffers){
    free(data);
    return 0;
  }

  data->buffers = buffers;
  data->channel = channel;

  segment->free = channel_segment_free;
  segment->mix = drain_segment_mix;
  segment->set_in = channel_segment_set_buffer;
  segment->info = drain_segment_info;
  segment->data = data;
  return 1;
}