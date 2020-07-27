#include "../internal.h"

struct distribute_data{
  struct mixed_buffer **out;
  size_t count;
  size_t size;
  struct mixed_buffer *in;
  size_t was_available;
};

int distribute_free(struct mixed_segment *segment){
  free_vector((struct vector *)segment->data);
  return 1;
}

int distribute_set_in(size_t field, size_t location, void *buffer, struct mixed_segment *segment){
  struct distribute_data *data = (struct distribute_data *)segment->data;
  
  switch(field){
  case MIXED_BUFFER:
    if(0 < location){
      mixed_err(MIXED_INVALID_LOCATION);
      return 0;
    }
    if(((struct mixed_buffer *)buffer)->_data){
      mixed_err(MIXED_BUFFER_ALLOCATED);
      return 0;
    }
    data->in = (struct mixed_buffer *)buffer;
    return 1;
  default:
    mixed_err(MIXED_INVALID_FIELD);
    return 0;
  }
}

int distribute_set_out(size_t field, size_t location, void *buffer, struct mixed_segment *segment){
  struct distribute_data *data = (struct distribute_data *)segment->data;

  switch(field){
  case MIXED_BUFFER:
    if(buffer){ // Add or set an element
      ((struct mixed_buffer *)buffer)->virtual = 1;
      if(location < data->count){
        data->out[location] = (struct mixed_buffer *)buffer;
      }else{
        return vector_add(buffer, (struct vector *)data);
      }
    }else{ // Remove an element
      if(data->count <= location){
        mixed_err(MIXED_INVALID_LOCATION);
        return 0;
      }
      mixed_free_buffer(data->out[location]);
      return vector_remove_pos(location, (struct vector *)data);
    }
    return 1;
  default:
    mixed_err(MIXED_INVALID_FIELD);
    return 0;
  }
}

int distribute_start(struct mixed_segment *segment){
  struct distribute_data *data = (struct distribute_data *)segment->data;
  struct mixed_buffer *in = data->in;

  if(!in){
    mixed_err(MIXED_BUFFER_MISSING);
    return 0;
  }

  for(size_t i=0; i<data->count; ++i){
    struct mixed_buffer *buffer = data->out[i];
    buffer->_data = in->_data;
    buffer->size = in->size;
  }

  data->was_available = 0;
  return 1;
}

int distribute_mix(struct mixed_segment *segment){
  struct distribute_data *data = (struct distribute_data *)segment->data;
  struct mixed_buffer *in = data->in;
  // FIXME: This explicitly does /not/ support reads from both bip regions at once.
  size_t max_available = mixed_buffer_available_read(data->out[0]);
  for(size_t i=1; i<data->count; ++i){
    max_available = MAX(max_available, mixed_buffer_available_read(data->out[i]));
  }
  mixed_buffer_finish_read(data->was_available - max_available, in);
  // Update virtual buffers with content of real buffer, disregarding R2.
  data->was_available = mixed_buffer_available_read(in);
  for(size_t i=0; i<data->count; ++i){
    struct mixed_buffer *buffer = data->out[i];
    buffer->r1_start = in->r1_start;
    buffer->r1_size = in->r1_size;
  }
  return 1;
}

int distribute_info(struct mixed_segment_info *info, struct mixed_segment *segment){
  struct distribute_data *data = (struct distribute_data *)segment->data;
  info->name = "distribute";
  info->description = "Multiplexes a buffer to multiple outputs to consume it from.";
  info->min_inputs = 1;
  info->max_inputs = 1;
  info->outputs = -1;
  
  struct mixed_segment_field_info *field = info->fields;
  set_info_field(field++, MIXED_BUFFER,
                 MIXED_BUFFER_POINTER, 1, MIXED_IN | MIXED_OUT | MIXED_SET,
                 "The buffer for audio data attached to the location.");
  clear_info_field(field++);
  return 1;
}

MIXED_EXPORT int mixed_make_segment_distribute(struct mixed_segment *segment){
  struct distribute_data *data = calloc(1, sizeof(struct distribute_data));
  if(!data){
    mixed_err(MIXED_OUT_OF_MEMORY);
    return 0;
  }
  
  segment->free = distribute_free;
  segment->mix = distribute_mix;
  segment->set_in = distribute_set_in;
  segment->set_out = distribute_set_out;
  segment->info = distribute_info;
  segment->data = data;
  return 1;
}
