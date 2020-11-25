#include "../include/visky/context.h"
#include "vklite2_utils.h"
#include <stdlib.h>



/*************************************************************************************************/
/*  Macros                                                                                       */
/*************************************************************************************************/

#define TO_KB(x) ((x) / (1024.0))



/*************************************************************************************************/
/*  Thread-safe FIFO queue                                                                       */
/*************************************************************************************************/

VklFifo vkl_fifo(int32_t capacity)
{
    log_trace("creating generic FIFO queue with a capacity of %d items", capacity);
    ASSERT(capacity >= 2);
    VklFifo fifo = {0};
    ASSERT(capacity <= VKL_MAX_FIFO_CAPACITY);
    fifo.capacity = capacity;

    if (pthread_mutex_init(&fifo.lock, NULL) != 0)
        log_error("mutex creation failed");
    if (pthread_cond_init(&fifo.cond, NULL) != 0)
        log_error("cond creation failed");

    return fifo;
}



void vkl_fifo_enqueue(VklFifo* fifo, void* item)
{
    ASSERT(fifo != NULL);
    pthread_mutex_lock(&fifo->lock);

    if ((fifo->head + 1) % fifo->capacity != fifo->tail)
    {
        log_trace("enqueue item, head %d, tail %d", fifo->head, fifo->tail);
        fifo->items[fifo->head] = item;
        fifo->head++;
        if (fifo->head >= fifo->capacity)
            fifo->head -= fifo->capacity;
    }
    else
    {
        log_error("FIFO queue is full, reseting it");
        fifo->head = 0;
        fifo->tail = 0;
    }

    ASSERT(0 <= fifo->head && fifo->head < fifo->capacity);
    pthread_cond_signal(&fifo->cond);
    pthread_mutex_unlock(&fifo->lock);
}



void* vkl_fifo_dequeue(VklFifo* fifo, bool wait)
{
    ASSERT(fifo != NULL);
    pthread_mutex_lock(&fifo->lock);

    // Wait until the queue is not empty.
    if (wait)
    {
        log_trace("waiting for the queue to be non-empty");
        while (fifo->head == fifo->tail)
            pthread_cond_wait(&fifo->cond, &fifo->lock);
    }

    // Empty queue.
    if (fifo->head == fifo->tail)
    {
        log_trace("FIFO queue was empty");
        // Don't forget to unlock the mutex before exiting this function.
        pthread_mutex_unlock(&fifo->lock);
        return NULL;
    }

    ASSERT(0 <= fifo->tail && fifo->tail < fifo->capacity);

    log_trace("dequeue item, head %d, tail %d", fifo->head, fifo->tail);
    void* item = fifo->items[fifo->tail];

    fifo->tail++;
    if (fifo->tail >= fifo->capacity)
        fifo->tail -= fifo->capacity;

    ASSERT(0 <= fifo->tail && fifo->tail < fifo->capacity);
    pthread_mutex_unlock(&fifo->lock);

    return item;
}



int vkl_fifo_size(VklFifo* fifo)
{
    ASSERT(fifo != NULL);
    pthread_mutex_lock(&fifo->lock);
    log_debug("head %d tail %d", fifo->head, fifo->tail);
    int size = fifo->head - fifo->tail;
    if (size < 0)
        size += fifo->capacity;
    ASSERT(0 <= size && size <= fifo->capacity);
    pthread_mutex_unlock(&fifo->lock);
    return size;
}



void vkl_fifo_discard(VklFifo* fifo, int max_size)
{
    ASSERT(fifo != NULL);
    if (max_size == 0)
        return;
    pthread_mutex_lock(&fifo->lock);
    int size = fifo->head - fifo->tail;
    if (size < 0)
        size += fifo->capacity;
    ASSERT(0 <= size && size <= fifo->capacity);
    if (size > max_size)
    {
        log_trace(
            "discarding %d items in the FIFO queue which is getting overloaded", size - max_size);
        fifo->tail = fifo->head - max_size;
        if (fifo->tail < 0)
            fifo->tail += fifo->capacity;
    }
    pthread_mutex_unlock(&fifo->lock);
}



void vkl_fifo_reset(VklFifo* fifo)
{
    ASSERT(fifo != NULL);
    pthread_mutex_lock(&fifo->lock);
    fifo->head = 0;
    fifo->tail = 0;
    pthread_cond_signal(&fifo->cond);
    pthread_mutex_unlock(&fifo->lock);
}



void vkl_fifo_destroy(VklFifo* fifo)
{
    ASSERT(fifo != NULL);
    pthread_mutex_destroy(&fifo->lock);
    pthread_cond_destroy(&fifo->cond);
}



/*************************************************************************************************/
/*  Context                                                                                      */
/*************************************************************************************************/

static void _context_default_queues(VklGpu* gpu, VklWindow* window)
{
    vkl_gpu_queue(gpu, VKL_QUEUE_TRANSFER, VKL_DEFAULT_QUEUE_TRANSFER);
    vkl_gpu_queue(gpu, VKL_QUEUE_COMPUTE, VKL_DEFAULT_QUEUE_COMPUTE);
    vkl_gpu_queue(gpu, VKL_QUEUE_RENDER, VKL_DEFAULT_QUEUE_RENDER);
    if (window != NULL)
        vkl_gpu_queue(gpu, VKL_QUEUE_PRESENT, VKL_DEFAULT_QUEUE_PRESENT);
}



static void _context_default_buffers(VklContext* context)
{
    // Create a predetermined set of buffers.
    VklBuffer* buffer = NULL;
    for (uint32_t i = 0; i < VKL_DEFAULT_BUFFER_COUNT; i++)
    {
        context->buffers[i] = vkl_buffer(context->gpu);
        buffer = &context->buffers[i];

        // All buffers may be accessed from these queues.
        vkl_buffer_queue_access(buffer, VKL_DEFAULT_QUEUE_TRANSFER);
        vkl_buffer_queue_access(buffer, VKL_DEFAULT_QUEUE_COMPUTE);
        vkl_buffer_queue_access(buffer, VKL_DEFAULT_QUEUE_RENDER);
    }

    VkBufferUsageFlagBits transferable =
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

    // Staging buffer
    buffer = &context->buffers[VKL_DEFAULT_BUFFER_STAGING];
    vkl_buffer_size(buffer, VKL_DEFAULT_BUFFER_STAGING_SIZE);
    vkl_buffer_usage(buffer, transferable);
    vkl_buffer_memory(
        buffer, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    vkl_buffer_create(buffer);

    // Vertex buffer
    buffer = &context->buffers[VKL_DEFAULT_BUFFER_VERTEX];
    vkl_buffer_size(buffer, VKL_DEFAULT_BUFFER_VERTEX_SIZE);
    vkl_buffer_usage(
        buffer,
        transferable | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    vkl_buffer_memory(buffer, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    vkl_buffer_create(buffer);

    // Index buffer
    buffer = &context->buffers[VKL_DEFAULT_BUFFER_INDEX];
    vkl_buffer_size(buffer, VKL_DEFAULT_BUFFER_INDEX_SIZE);
    vkl_buffer_usage(buffer, transferable | VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
    vkl_buffer_memory(buffer, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    vkl_buffer_create(buffer);

    // Storage buffer
    buffer = &context->buffers[VKL_DEFAULT_BUFFER_STORAGE];
    vkl_buffer_size(buffer, VKL_DEFAULT_BUFFER_STORAGE_SIZE);
    vkl_buffer_usage(buffer, transferable | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    vkl_buffer_memory(buffer, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    vkl_buffer_create(buffer);

    // Uniform buffer
    buffer = &context->buffers[VKL_DEFAULT_BUFFER_UNIFORM];
    vkl_buffer_size(buffer, VKL_DEFAULT_BUFFER_UNIFORM_SIZE);
    vkl_buffer_usage(buffer, transferable | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
    vkl_buffer_memory(buffer, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    vkl_buffer_create(buffer);
}



static void _destroy_resources(VklContext* context)
{
    ASSERT(context != NULL);

    log_trace("context destroy buffers");
    for (uint32_t i = 0; i < context->max_buffers; i++)
    {
        if (context->buffers[i].obj.status == VKL_OBJECT_STATUS_NONE)
            break;
        vkl_buffer_destroy(&context->buffers[i]);
    }

    log_trace("context destroy sets of images");
    for (uint32_t i = 0; i < context->max_images; i++)
    {
        if (context->images[i].obj.status == VKL_OBJECT_STATUS_NONE)
            break;
        vkl_images_destroy(&context->images[i]);
    }

    log_trace("context destroy samplers");
    for (uint32_t i = 0; i < context->max_samplers; i++)
    {
        if (context->samplers[i].obj.status == VKL_OBJECT_STATUS_NONE)
            break;
        vkl_sampler_destroy(&context->samplers[i]);
    }

    log_trace("context destroy computes");
    for (uint32_t i = 0; i < context->max_computes; i++)
    {
        if (context->computes[i].obj.status == VKL_OBJECT_STATUS_NONE)
            break;
        vkl_compute_destroy(&context->computes[i]);
    }
}



VklContext* vkl_context(VklGpu* gpu, VklWindow* window)
{
    ASSERT(gpu != NULL);
    ASSERT(!is_obj_created(&gpu->obj));
    log_trace("creating context");

    VklContext* context = calloc(1, sizeof(VklContext));
    context->gpu = gpu;

    // Allocate memory for buffers, textures, and computes.
    INSTANCES_INIT(
        VklBuffer, context, buffers, max_buffers, VKL_MAX_BUFFERS, VKL_OBJECT_TYPE_BUFFER)
    context->allocated_sizes = calloc(context->max_buffers, sizeof(VkDeviceSize));

    INSTANCES_INIT(
        VklTexture, context, textures, max_textures, VKL_MAX_TEXTURES, VKL_OBJECT_TYPE_TEXTURE)

    INSTANCES_INIT(
        VklImages, context, images, max_images, VKL_MAX_TEXTURES, VKL_OBJECT_TYPE_IMAGES)

    INSTANCES_INIT(
        VklSampler, context, samplers, max_samplers, VKL_MAX_TEXTURES, VKL_OBJECT_TYPE_SAMPLER)

    INSTANCES_INIT(
        VklCompute, context, computes, max_computes, VKL_MAX_COMPUTES, VKL_OBJECT_TYPE_COMPUTE)

    // Specify the default queues.
    _context_default_queues(gpu, window);

    // Create the GPU after the default queues have been set.
    if (!is_obj_created(&gpu->obj))
    {
        VkSurfaceKHR surface = VK_NULL_HANDLE;
        if (window != NULL)
            surface = window->surface;
        vkl_gpu_create(gpu, surface);
    }

    // Create the default buffers.
    _context_default_buffers(context);

    context->transfer_cmd = vkl_commands(gpu, VKL_DEFAULT_QUEUE_TRANSFER, 1);
    context->fifo = vkl_fifo(VKL_MAX_FIFO_CAPACITY);

    gpu->context = context;
    obj_created(&context->obj);

    return context;
}



void vkl_context_reset(VklContext* context)
{
    ASSERT(context != NULL);
    log_trace("reset the context");
    _destroy_resources(context);
    _context_default_buffers(context);
}



void vkl_context_destroy(VklContext* context)
{
    if (context == NULL)
    {
        log_error("skip destruction of null context");
        return;
    }
    log_trace("destroying context");
    ASSERT(context != NULL);
    ASSERT(context->gpu != NULL);

    // Destroy the buffers, images, samplers, textures, computes.
    _destroy_resources(context);

    // Free the allocated memory.
    INSTANCES_DESTROY(context->buffers)
    INSTANCES_DESTROY(context->images)
    INSTANCES_DESTROY(context->samplers)
    INSTANCES_DESTROY(context->computes)
    INSTANCES_DESTROY(context->textures);
    FREE(context->allocated_sizes);

    vkl_fifo_destroy(&context->fifo);
}



/*************************************************************************************************/
/*  Buffer allocation                                                                            */
/*************************************************************************************************/

VklBufferRegions vkl_alloc_buffers(
    VklContext* context, uint32_t buffer_idx, uint32_t buffer_count, VkDeviceSize size)
{
    ASSERT(context != NULL);
    ASSERT(context->gpu != NULL);
    ASSERT(buffer_count > 0);
    ASSERT(size > 0);

    VkDeviceSize alignment = 0;
    VkDeviceSize offset = context->allocated_sizes[buffer_idx];
    if (buffer_idx == VKL_DEFAULT_BUFFER_UNIFORM)
    {
        // alignment = get_alignment(
        //     size, context->gpu->device_properties.limits.minUniformBufferOffsetAlignment);
        alignment = context->gpu->device_properties.limits.minUniformBufferOffsetAlignment;
        ASSERT(offset % alignment == 0); // offset should be already aligned
    }

    VklBufferRegions regions =
        vkl_buffer_regions(&context->buffers[buffer_idx], buffer_count, offset, size, alignment);
    VkDeviceSize alsize = regions.aligned_size;
    if (alsize == 0)
        alsize = size;
    ASSERT(alsize > 0);

    if (buffer_idx >= context->max_buffers || !is_obj_created(&context->buffers[buffer_idx].obj))
    {
        log_error("invalid buffer #%d", buffer_idx);
        return regions;
    }

    // Check alignment for uniform buffers.
    if (buffer_idx == VKL_DEFAULT_BUFFER_UNIFORM)
    {
        ASSERT(alignment > 0);
        ASSERT(alsize % alignment == 0);
        for (uint32_t i = 0; i < buffer_count; i++)
            ASSERT(regions.offsets[i] % alignment == 0);
    }

    // Need to reallocate?
    if (offset + alsize * buffer_count > regions.buffer->size)
    {
        VkDeviceSize new_size = regions.buffer->size * 2;
        log_info("reallocating buffer #%d to %.3f KB", buffer_idx, TO_KB(new_size));
        vkl_buffer_resize(
            regions.buffer, new_size, VKL_DEFAULT_QUEUE_TRANSFER, &context->transfer_cmd);
    }

    log_trace(
        "allocating %d buffers with size %d bytes (aligned size %d bytes)", //
        buffer_count, size, alsize);
    ASSERT(offset + alsize * buffer_count <= regions.buffer->size);
    context->allocated_sizes[buffer_idx] += alsize * buffer_count;

    ASSERT(regions.offsets[buffer_count - 1] + alsize == context->allocated_sizes[buffer_idx]);
    return regions;
}



/*************************************************************************************************/
/*  Compute                                                                                      */
/*************************************************************************************************/

VklCompute* vkl_new_compute(VklContext* context, const char* shader_path)
{
    ASSERT(context != NULL);
    ASSERT(shader_path != NULL);

    INSTANCE_NEW(VklCompute, compute, context->computes, context->max_computes);

    *compute = vkl_compute(context->gpu, shader_path);

    return compute;
}



/*************************************************************************************************/
/*  Texture                                                                                      */
/*************************************************************************************************/

static VkImageType image_type_from_dims(uint32_t dims)
{
    switch (dims)
    {
    case 1:
        return VK_IMAGE_TYPE_1D;
        break;
    case 2:
        return VK_IMAGE_TYPE_2D;
        break;
    case 3:
        return VK_IMAGE_TYPE_3D;
        break;

    default:
        break;
    }
    log_error("invalid image dimensions %d", dims);
    return VK_IMAGE_TYPE_2D;
}



VklTexture* vkl_new_texture(VklContext* context, uint32_t dims, uvec3 size, VkFormat format)
{
    ASSERT(context != NULL);

    INSTANCE_NEW(VklTexture, texture, context->textures, context->max_textures);
    INSTANCE_NEW(VklImages, image, context->images, context->max_images);
    INSTANCE_NEW(VklSampler, sampler, context->samplers, context->max_samplers);

    texture->context = context;
    *image = vkl_images(context->gpu, image_type_from_dims(dims), 1);
    *sampler = vkl_sampler(context->gpu);

    texture->image = image;
    texture->sampler = sampler;

    // Create the image.
    vkl_images_format(image, format);
    vkl_images_size(image, size[0], size[1], size[2]);
    vkl_images_tiling(image, VK_IMAGE_TILING_OPTIMAL);
    vkl_images_layout(image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    vkl_images_usage(
        image, //
        VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
            VK_IMAGE_USAGE_TRANSFER_DST_BIT);
    vkl_images_memory(image, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    vkl_images_queue_access(image, VKL_DEFAULT_QUEUE_TRANSFER);
    vkl_images_queue_access(image, VKL_DEFAULT_QUEUE_COMPUTE);
    vkl_images_queue_access(image, VKL_DEFAULT_QUEUE_RENDER);
    vkl_images_create(image);

    // Create the sampler.
    vkl_sampler_min_filter(sampler, VK_FILTER_NEAREST);
    vkl_sampler_mag_filter(sampler, VK_FILTER_NEAREST);
    vkl_sampler_address_mode(sampler, VKL_TEXTURE_AXIS_U, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
    vkl_sampler_address_mode(sampler, VKL_TEXTURE_AXIS_V, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
    vkl_sampler_address_mode(sampler, VKL_TEXTURE_AXIS_V, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
    vkl_sampler_create(sampler);

    obj_created(&texture->obj);

    return texture;
}



void vkl_texture_resize(VklTexture* texture, uvec3 size)
{
    ASSERT(texture != NULL);
    ASSERT(texture->image != NULL);

    vkl_images_resize(texture->image, size[0], size[1], size[2]);
}



void vkl_texture_filter(VklTexture* texture, VklFilterType type, VkFilter filter)
{
    ASSERT(texture != NULL);
    ASSERT(texture->sampler != NULL);

    switch (type)
    {
    case VKL_FILTER_MIN:
        vkl_sampler_min_filter(texture->sampler, filter);
        break;
    case VKL_FILTER_MAX:
        vkl_sampler_mag_filter(texture->sampler, filter);
        break;
    default:
        log_error("invalid filter type %d", type);
        break;
    }
    vkl_sampler_destroy(texture->sampler);
    vkl_sampler_create(texture->sampler);
}



void vkl_texture_address_mode(
    VklTexture* texture, VklTextureAxis axis, VkSamplerAddressMode address_mode)
{
    ASSERT(texture != NULL);
    ASSERT(texture->sampler != NULL);

    vkl_sampler_address_mode(texture->sampler, axis, address_mode);

    vkl_sampler_destroy(texture->sampler);
    vkl_sampler_create(texture->sampler);
}



void vkl_texture_destroy(VklTexture* texture)
{
    ASSERT(texture != NULL);
    vkl_images_destroy(texture->image);
    vkl_sampler_destroy(texture->sampler);

    texture->image = NULL;
    texture->sampler = NULL;
    obj_destroyed(&texture->obj);
}



/*************************************************************************************************/
/*  Data transfers utils                                                                         */
/*************************************************************************************************/

static void process_texture_upload(VklContext* context, VklTransfer tr)
{
    ASSERT(context != NULL);

    VklGpu* gpu = context->gpu;
    ASSERT(gpu != NULL);

    ASSERT(tr.type == VKL_TRANSFER_TEXTURE_UPLOAD);

    // Wait for the transfer queue to be idle.
    vkl_queue_wait(gpu, VKL_DEFAULT_QUEUE_TRANSFER);

    // Take the staging buffer.
    VklBuffer* staging = &context->buffers[VKL_DEFAULT_BUFFER_STAGING];

    // Size of the buffer to transfer.
    VkDeviceSize size = tr.u.tex.size;

    // Transfer from the CPU to the GPU staging buffer.
    vkl_buffer_upload(staging, 0, size, (const void*)tr.u.tex.data);

    // Take transfer cmd buf.
    VklCommands* cmds = &context->transfer_cmd;
    vkl_cmd_reset(cmds, 0);
    vkl_cmd_begin(cmds, 0);

    // Image transition.
    VklBarrier barrier = vkl_barrier(gpu);
    vkl_barrier_stages(&barrier, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
    ASSERT(tr.u.tex.texture != NULL);
    ASSERT(tr.u.tex.texture->image != NULL);
    vkl_barrier_images(&barrier, tr.u.tex.texture->image);
    vkl_barrier_images_layout(
        &barrier, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    vkl_barrier_images_access(&barrier, 0, VK_ACCESS_TRANSFER_WRITE_BIT);
    vkl_cmd_barrier(cmds, 0, &barrier);

    // Copy to staging buffer
    vkl_cmd_copy_buffer_to_image(cmds, 0, staging, tr.u.tex.texture->image);

    // Image transition.
    vkl_barrier_images_layout(
        &barrier, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, tr.u.tex.texture->image->layout);
    vkl_barrier_images_access(&barrier, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_MEMORY_READ_BIT);
    vkl_cmd_barrier(cmds, 0, &barrier);

    vkl_cmd_end(cmds, 0);

    // Wait for the render queue to be idle.
    vkl_queue_wait(gpu, VKL_DEFAULT_QUEUE_RENDER);

    // Submit the commands to the transfer queue.
    VklSubmit submit = vkl_submit(gpu);
    vkl_submit_commands(&submit, cmds);
    vkl_submit_send(&submit, 0, NULL, 0);

    // Wait for the transfer queue to be idle.
    vkl_queue_wait(gpu, VKL_DEFAULT_QUEUE_TRANSFER);
}



static void process_texture_download(VklContext* context, VklTransfer tr)
{
    ASSERT(context != NULL);

    VklGpu* gpu = context->gpu;
    ASSERT(gpu != NULL);

    ASSERT(tr.type == VKL_TRANSFER_TEXTURE_DOWNLOAD);

    // Take the staging buffer.
    VklBuffer* staging = &context->buffers[VKL_DEFAULT_BUFFER_STAGING];

    // Take transfer cmd buf.
    VklCommands* cmds = &context->transfer_cmd;
    vkl_cmd_reset(cmds, 0);
    vkl_cmd_begin(cmds, 0);

    // Image transition.
    VklBarrier barrier = vkl_barrier(gpu);
    vkl_barrier_stages(&barrier, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
    ASSERT(tr.u.tex.texture != NULL);
    ASSERT(tr.u.tex.texture->image != NULL);
    vkl_barrier_images(&barrier, tr.u.tex.texture->image);
    vkl_barrier_images_layout(
        &barrier, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    vkl_barrier_images_access(&barrier, 0, VK_ACCESS_TRANSFER_READ_BIT);
    vkl_cmd_barrier(cmds, 0, &barrier);

    // Copy to staging buffer
    vkl_cmd_copy_image_to_buffer(cmds, 0, tr.u.tex.texture->image, staging);

    // Image transition.
    vkl_barrier_images_layout(
        &barrier, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, tr.u.tex.texture->image->layout);
    vkl_barrier_images_access(&barrier, VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_MEMORY_READ_BIT);
    vkl_cmd_barrier(cmds, 0, &barrier);

    vkl_cmd_end(cmds, 0);

    // Wait for the render queue to be idle.
    vkl_queue_wait(gpu, VKL_DEFAULT_QUEUE_RENDER);

    // Submit the commands to the transfer queue.
    VklSubmit submit = vkl_submit(gpu);
    vkl_submit_commands(&submit, cmds);
    vkl_submit_send(&submit, 0, NULL, 0);

    // Wait for the transfer queue to be idle.
    vkl_queue_wait(gpu, VKL_DEFAULT_QUEUE_TRANSFER);

    // Transfer from the CPU to the GPU staging buffer.
    vkl_buffer_download(staging, 0, tr.u.tex.size, tr.u.tex.data);
}



static void process_buffer_upload(VklContext* context, VklTransfer tr)
{
    ASSERT(context != NULL);

    VklGpu* gpu = context->gpu;
    ASSERT(gpu != NULL);

    ASSERT(tr.type == VKL_TRANSFER_BUFFER_UPLOAD);

    // Wait for the transfer queue to be idle.
    vkl_queue_wait(gpu, VKL_DEFAULT_QUEUE_TRANSFER);

    // Take the staging buffer.
    VklBuffer* staging = &context->buffers[VKL_DEFAULT_BUFFER_STAGING];

    // Size of the buffer to transfer.
    VkDeviceSize region_size = tr.u.buf.size;
    ASSERT(region_size > 0);

    VkDeviceSize alsize = tr.u.buf.regions.aligned_size;
    if (alsize == 0)
        alsize = region_size;
    ASSERT(alsize > 0);

    uint32_t n = tr.u.buf.regions.count;

    // Copy the data as many times as there are buffer regions, and make sure the array is
    // aligned if using a UNIFORM buffer.
    void* repeated = aligned_repeat(region_size, tr.u.buf.data, n, tr.u.buf.regions.alignment);
    // Transfer from the CPU to the GPU staging buffer.
    VkDeviceSize total_size = alsize * n;
    vkl_buffer_upload(staging, 0, total_size, repeated);
    FREE(repeated);

    // Take transfer cmd buf.
    VklCommands* cmds = &context->transfer_cmd;
    vkl_cmd_reset(cmds, 0);
    vkl_cmd_begin(cmds, 0);

    // Determine the offset in the target buffer.
    VkDeviceSize init_offset = tr.u.buf.regions.offsets[0];
    VkDeviceSize sub_offset = tr.u.buf.offset;
    ASSERT(tr.u.buf.regions.buffer != VK_NULL_HANDLE);
    VkBufferCopy* regions = calloc(n, sizeof(VkBufferCopy));
    for (uint32_t i = 0; i < n; i++)
    {
        regions[i].size = region_size;
        regions[i].srcOffset = sub_offset + i * alsize;
        regions[i].dstOffset = init_offset + sub_offset + i * alsize;
    }
    vkCmdCopyBuffer(
        cmds->cmds[0], staging->buffer, tr.u.buf.regions.buffer->buffer, //
        tr.u.buf.regions.count, regions);
    FREE(regions);

    vkl_cmd_end(cmds, 0);

    // Wait for the render queue to be idle.
    vkl_queue_wait(gpu, VKL_DEFAULT_QUEUE_RENDER);

    // Submit the commands to the transfer queue.
    VklSubmit submit = vkl_submit(gpu);
    vkl_submit_commands(&submit, cmds);
    vkl_submit_send(&submit, 0, NULL, 0);

    // Wait for the transfer queue to be idle.
    vkl_queue_wait(gpu, VKL_DEFAULT_QUEUE_TRANSFER);
}



static void process_buffer_download(VklContext* context, VklTransfer tr)
{
    ASSERT(context != NULL);

    VklGpu* gpu = context->gpu;
    ASSERT(gpu != NULL);

    ASSERT(tr.type == VKL_TRANSFER_BUFFER_DOWNLOAD);

    // Take the staging buffer.
    VklBuffer* staging = &context->buffers[VKL_DEFAULT_BUFFER_STAGING];

    // Take transfer cmd buf.
    VklCommands* cmds = &context->transfer_cmd;
    vkl_cmd_reset(cmds, 0);
    vkl_cmd_begin(cmds, 0);

    // Size of the buffer to transfer.
    VkDeviceSize size = tr.u.buf.size;

    // Determine the offset in the source buffer.
    // Should be consecutive offsets.
    VkDeviceSize offset = tr.u.buf.regions.offsets[0];
    uint32_t n_regions = tr.u.buf.regions.count;
    for (uint32_t i = 1; i < n_regions; i++)
    {
        ASSERT(tr.u.buf.regions.offsets[i] == offset + i * size);
    }
    // Take into account the transfer offset.
    offset += tr.u.buf.offset;

    // Copy to staging buffer
    ASSERT(tr.u.buf.regions.buffer != 0);
    vkl_cmd_copy_buffer(cmds, 0, tr.u.buf.regions.buffer, offset, staging, 0, size * n_regions);
    vkl_cmd_end(cmds, 0);

    // Wait for the render queue to be idle.
    vkl_queue_wait(gpu, VKL_DEFAULT_QUEUE_RENDER);

    // Submit the commands to the transfer queue.
    VklSubmit submit = vkl_submit(gpu);
    vkl_submit_commands(&submit, cmds);
    vkl_submit_send(&submit, 0, NULL, 0);

    // Wait for the transfer queue to be idle.
    vkl_queue_wait(gpu, VKL_DEFAULT_QUEUE_TRANSFER);

    // Transfer from the CPU to the GPU staging buffer.
    vkl_buffer_download(staging, 0, size, tr.u.buf.data);
}



static void process_buffer_copy(VklContext* context, VklTransfer tr)
{
    ASSERT(context != NULL);

    VklGpu* gpu = context->gpu;
    ASSERT(gpu != NULL);

    ASSERT(tr.type == VKL_TRANSFER_BUFFER_COPY);
    VklBufferRegions* src = &tr.u.buf_copy.src;
    VklBufferRegions* dst = &tr.u.buf_copy.dst;
    ASSERT(src->count == dst->count);

    VkDeviceSize size = tr.u.buf_copy.size;
    VkDeviceSize src_offset = tr.u.buf_copy.src_offset;
    VkDeviceSize dst_offset = tr.u.buf_copy.dst_offset;

    // Take transfer cmd buf.
    VklCommands* cmds = &context->transfer_cmd;
    vkl_cmd_reset(cmds, 0);
    vkl_cmd_begin(cmds, 0);

    // Copy buffer command.
    VkBufferCopy* regions = (VkBufferCopy*)calloc(src->count, sizeof(VkBufferCopy));
    for (uint32_t i = 0; i < src->count; i++)
    {
        regions[i].size = size;
        regions[i].srcOffset = src->offsets[i] + src_offset;
        regions[i].dstOffset = dst->offsets[i] + dst_offset;
    }
    vkCmdCopyBuffer(cmds->cmds[0], src->buffer->buffer, dst->buffer->buffer, src->count, regions);

    vkl_cmd_end(cmds, 0);
    FREE(regions);

    // Wait for the render queue to be idle.
    vkl_queue_wait(gpu, VKL_DEFAULT_QUEUE_RENDER);

    // Submit the commands to the transfer queue.
    VklSubmit submit = vkl_submit(gpu);
    vkl_submit_commands(&submit, cmds);
    vkl_submit_send(&submit, 0, NULL, 0);

    // Wait for the transfer queue to be idle.
    vkl_queue_wait(gpu, VKL_DEFAULT_QUEUE_TRANSFER);
}



static void process_texture_copy(VklContext* context, VklTransfer tr)
{
    ASSERT(context != NULL);

    VklGpu* gpu = context->gpu;
    ASSERT(gpu != NULL);

    ASSERT(tr.type == VKL_TRANSFER_TEXTURE_COPY);
    VklTexture* src = tr.u.tex_copy.src;
    VklTexture* dst = tr.u.tex_copy.dst;

    // Take transfer cmd buf.
    VklCommands* cmds = &context->transfer_cmd;
    vkl_cmd_reset(cmds, 0);
    vkl_cmd_begin(cmds, 0);

    // Copy texture command.
    VkImageCopy copy = {0};
    copy.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copy.srcSubresource.layerCount = 1;
    copy.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copy.dstSubresource.layerCount = 1;
    copy.extent.width = tr.u.tex_copy.shape[0];
    copy.extent.height = tr.u.tex_copy.shape[1];
    copy.extent.depth = tr.u.tex_copy.shape[2];
    copy.srcOffset.x = (int32_t)tr.u.tex_copy.src_offset[0];
    copy.srcOffset.y = (int32_t)tr.u.tex_copy.src_offset[1];
    copy.srcOffset.z = (int32_t)tr.u.tex_copy.src_offset[2];
    copy.dstOffset.x = (int32_t)tr.u.tex_copy.dst_offset[0];
    copy.dstOffset.y = (int32_t)tr.u.tex_copy.dst_offset[1];
    copy.dstOffset.z = (int32_t)tr.u.tex_copy.dst_offset[2];
    vkCmdCopyImage(
        cmds->cmds[0],                             //
        src->image->images[0], src->image->layout, //
        dst->image->images[0], dst->image->layout, 1, &copy);

    vkl_cmd_end(cmds, 0);

    // Wait for the render queue to be idle.
    vkl_queue_wait(gpu, VKL_DEFAULT_QUEUE_RENDER);

    // Submit the commands to the transfer queue.
    VklSubmit submit = vkl_submit(gpu);
    vkl_submit_commands(&submit, cmds);
    vkl_submit_send(&submit, 0, NULL, 0);

    // Wait for the transfer queue to be idle.
    vkl_queue_wait(gpu, VKL_DEFAULT_QUEUE_TRANSFER);
}



static int process_transfer(VklContext* context, VklTransfer tr)
{
    ASSERT(context != NULL);
    switch (tr.type)
    {
    case VKL_TRANSFER_NONE:
        return 1;
        break;
    case VKL_TRANSFER_TEXTURE_UPLOAD:
        process_texture_upload(context, tr);
        break;
    case VKL_TRANSFER_TEXTURE_DOWNLOAD:
        process_texture_download(context, tr);
        break;
    case VKL_TRANSFER_BUFFER_UPLOAD:
        process_buffer_upload(context, tr);
        break;
    case VKL_TRANSFER_BUFFER_DOWNLOAD:
        process_buffer_download(context, tr);
        break;
    case VKL_TRANSFER_BUFFER_COPY:
        process_buffer_copy(context, tr);
        break;
    case VKL_TRANSFER_TEXTURE_COPY:
        process_texture_copy(context, tr);
        break;
    default:
        log_error("unknown transfer type %d", tr.type);
        break;
    }
    return 0;
}



/*************************************************************************************************/
/*  Transfer queue                                                                               */
/*************************************************************************************************/

static void fifo_enqueue(VklContext* ctx, VklTransfer transfer)
{
    ASSERT(ctx != NULL);
    VklFifo* fifo = &ctx->fifo;
    ASSERT(0 <= fifo->head && fifo->head < fifo->capacity);
    ctx->transfers[fifo->head] = transfer;
    vkl_fifo_enqueue(fifo, &ctx->transfers[fifo->head]);
}



static VklTransfer fifo_dequeue(VklContext* ctx, bool wait)
{
    ASSERT(ctx != NULL);
    VklFifo* fifo = &ctx->fifo;
    VklTransfer* item = vkl_fifo_dequeue(fifo, wait);
    if (item == NULL)
        return (VklTransfer){0};
    ASSERT(item != NULL);
    return *item;
}



static VklTransfer enqueue_texture_transfer(
    VklContext* ctx, VklDataTransferType type, VklTexture* texture, uvec3 offset, uvec3 shape,
    VkDeviceSize size, void* data)
{
    // Create the transfer object.
    VklTransfer tr = {0};
    tr.type = type;
    for (uint32_t i = 0; i < 3; i++)
    {
        tr.u.tex.shape[i] = shape[i];
        tr.u.tex.offset[i] = offset[i];
    }
    tr.u.tex.size = size;
    tr.u.tex.data = data;
    tr.u.tex.texture = texture;

    fifo_enqueue(ctx, tr);
    if (ctx->transfer_mode == VKL_TRANSFER_MODE_SYNC)
        process_transfer(ctx, tr);

    return tr;
}



static VklTransfer enqueue_regions_transfer(
    VklContext* ctx, VklDataTransferType type, VklBufferRegions regions, VkDeviceSize offset,
    VkDeviceSize size, void* data)
{
    // Create the transfer object.
    VklTransfer tr = {0};
    tr.type = type;
    tr.u.buf.regions = regions;
    tr.u.buf.offset = offset;
    tr.u.buf.size = size;
    tr.u.buf.data = data;

    fifo_enqueue(ctx, tr);
    if (ctx->transfer_mode == VKL_TRANSFER_MODE_SYNC)
        process_transfer(ctx, tr);

    return tr;
}



void vkl_transfer_mode(VklContext* context, VklTransferMode mode)
{
    ASSERT(context != NULL);
    context->transfer_mode = mode;
}



void vkl_transfer_loop(VklContext* context, bool wait)
{
    ASSERT(context != NULL);
    VklTransfer tr = {0};
    int res = 0;
    uint64_t counter = 0;
    while (res == 0)
    {
        log_trace("transfer loop awaits for transfer task, iteration %d...", counter);
        // wait until a transfer task is available
        tr = fifo_dequeue(context, wait);
        log_trace("transfer task dequeued, processing it...");
        // process the dequeued task
        res = process_transfer(context, tr);
        counter++;
    }
    log_trace("end transfer loop");
}



// Safe wait on a background thread until the transfer queue is empty. Periodically check the queue
// size.
void vkl_transfer_wait(VklContext* context, int poll_period)
{
    ASSERT(context != NULL);
    if (poll_period == 0)
        poll_period = VKL_TRANSFER_POLL_PERIOD;
    ASSERT(poll_period > 0);
    int size = 0;
    log_trace("waiting until the transfer queue is empty...");
    while (true)
    {
        size = vkl_fifo_size(&context->fifo);
        if (size == 0)
            break;
        vkl_sleep(poll_period);
    }
    log_trace("the transfer queue is empty, stop waiting");
}



void vkl_transfer_reset(VklContext* context)
{
    ASSERT(context != NULL);
    vkl_fifo_reset(&context->fifo);
}



void vkl_transfer_stop(VklContext* context)
{
    ASSERT(context != NULL);
    // Enqueue a special object that causes the dequeue loop to end.
    VklTransfer tr = {0};
    tr.type = VKL_TRANSFER_NONE;
    fifo_enqueue(context, tr);
}



/*************************************************************************************************/
/*  Data transfers                                                                               */
/*************************************************************************************************/

void vkl_texture_upload_region(
    VklContext* context, VklTexture* texture, uvec3 offset, uvec3 shape, VkDeviceSize size,
    void* data)
{
    ASSERT(texture != NULL);
    ASSERT(context != NULL);
    enqueue_texture_transfer(
        context, VKL_TRANSFER_TEXTURE_UPLOAD, texture, offset, shape, size, data);
}



void vkl_texture_upload(VklContext* context, VklTexture* texture, VkDeviceSize size, void* data)
{
    ASSERT(texture != NULL);
    ASSERT(context != NULL);

    uvec3 shape = {0};
    shape[0] = texture->image->width;
    shape[1] = texture->image->height;
    shape[2] = texture->image->depth;
    vkl_texture_upload_region(context, texture, (uvec3){0, 0, 0}, shape, size, data);
}



void vkl_texture_download_region(
    VklContext* context, VklTexture* texture, uvec3 offset, uvec3 shape, VkDeviceSize size,
    void* data)
{
    ASSERT(texture != NULL);
    ASSERT(context != NULL);
    enqueue_texture_transfer(
        context, VKL_TRANSFER_TEXTURE_DOWNLOAD, texture, offset, shape, size, data);
}



void vkl_texture_download(VklContext* context, VklTexture* texture, VkDeviceSize size, void* data)
{
    ASSERT(texture != NULL);
    ASSERT(context != NULL);

    uvec3 shape = {0};
    shape[0] = texture->image->width;
    shape[1] = texture->image->height;
    shape[2] = texture->image->depth;
    vkl_texture_download_region(context, texture, (uvec3){0, 0, 0}, shape, size, data);
}



void vkl_buffer_regions_upload(
    VklContext* context, VklBufferRegions* regions, VkDeviceSize offset, VkDeviceSize size,
    void* data)
{
    ASSERT(regions != NULL);
    ASSERT(context != NULL);
    enqueue_regions_transfer(context, VKL_TRANSFER_BUFFER_UPLOAD, *regions, offset, size, data);
}



void vkl_buffer_regions_download(
    VklContext* context, VklBufferRegions* regions, VkDeviceSize offset, VkDeviceSize size,
    void* data)
{
    ASSERT(regions != NULL);
    ASSERT(context != NULL);
    enqueue_regions_transfer(context, VKL_TRANSFER_BUFFER_DOWNLOAD, *regions, offset, size, data);
}



void vkl_buffer_regions_copy(
    VklContext* context,                           //
    VklBufferRegions src, VkDeviceSize src_offset, //
    VklBufferRegions dst, VkDeviceSize dst_offset, //
    VkDeviceSize size)
{
    ASSERT(context != NULL);
    ASSERT(src.buffer != NULL);
    ASSERT(dst.buffer != NULL);

    // Create the transfer object.
    VklTransfer tr = {0};
    tr.type = VKL_TRANSFER_BUFFER_COPY;
    tr.u.buf_copy.src = src;
    tr.u.buf_copy.dst = dst;
    tr.u.buf_copy.src_offset = src_offset;
    tr.u.buf_copy.dst_offset = dst_offset;
    tr.u.buf_copy.size = size;

    fifo_enqueue(context, tr);
    if (context->transfer_mode == VKL_TRANSFER_MODE_SYNC)
        process_transfer(context, tr);
}



void vkl_texture_copy(
    VklContext* context, VklTexture* src, uvec3 src_offset, VklTexture* dst, uvec3 dst_offset,
    uvec3 shape)
{
    ASSERT(context != NULL);
    ASSERT(src != NULL);
    ASSERT(dst != NULL);

    // Create the transfer object.
    VklTransfer tr = {0};
    tr.type = VKL_TRANSFER_TEXTURE_COPY;
    tr.u.tex_copy.src = src;
    tr.u.tex_copy.dst = dst;
    memcpy(tr.u.tex_copy.src_offset, src_offset, sizeof(uvec3));
    memcpy(tr.u.tex_copy.dst_offset, dst_offset, sizeof(uvec3));
    memcpy(tr.u.tex_copy.shape, shape, sizeof(uvec3));

    fifo_enqueue(context, tr);
    if (context->transfer_mode == VKL_TRANSFER_MODE_SYNC)
        process_transfer(context, tr);
}
