/* Copyright (c) 2019, Arm Limited and Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 the "License";
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include "common/helpers.h"
#include "common/vk_common.h"
#include "core/command_buffer.h"
#include "core/command_pool.h"
#include "core/descriptor_set.h"
#include "core/descriptor_set_layout.h"
#include "core/framebuffer.h"
#include "core/pipeline.h"
#include "core/pipeline_layout.h"
#include "core/queue.h"
#include "core/render_pass.h"
#include "core/shader_module.h"
#include "core/swapchain.h"
#include "rendering/pipeline_state.h"
#include "rendering/render_frame.h"
#include "rendering/render_target.h"
#include "resource_cache.h"

namespace vkb
{
/**
 * @brief RenderContext acts as a frame manager for the sample, with a lifetime that is the
 * same as that of the Application itself. It acts as a container for RenderFrame objects,
 * swapping between them (begin_frame, end_frame) and forwarding requests for Vulkan resources
 * to the active frame. Note that it's guaranteed that there is always an active frame.
 * More than one frame can be in-flight in the GPU, thus the need for per-frame resources.
 *
 * It requires a Device to be valid on creation and will take control of the Swapchain, so a
 * RenderFrame can be created for each Swapchain image.
 *
 * A RenderContext can be extended for headless mode (i.e. not presenting rendered images to
 * a display) by removing the swapchain part and overriding begin_frame and end_frame.
 */
class RenderContext : public NonCopyable
{
  public:
	/**
	 * @brief Constructor
	 * @param device A valid device
	 * @param surface A surface, VK_NULL_HANDLE if in headless mode
	 */
	RenderContext(Device &device, VkSurfaceKHR surface);

	virtual ~RenderContext() = default;

	/**
	 * @brief Requests to set the present mode of the swapchain, must be called before prepare
	 */
	void request_present_mode(const VkPresentModeKHR present_mode);

	/**
	 * @brief Requests to set a specific image format for the swapchain
	 */
	void request_image_format(const VkFormat format);

	/**
	 * @brief Sets the order in which the swapchain prioritizes selecting its present mode
	 */
	void set_present_mode_priority(const std::vector<VkPresentModeKHR> &present_mode_priority_list);

	/**
	 * @brief Sets the order in which the swapchain prioritizes selecting its surface format
	 */
	void set_surface_format_priority(const std::vector<VkSurfaceFormatKHR> &surface_format_priority_list);

	/**
	 * @brief Creates the necessary components to allow the render context to be rendered
	 */
	void prepare(uint16_t command_pools_per_frame = 1, RenderTarget::CreateFunc create_render_target_func = RenderTarget::DEFAULT_CREATE_FUNC);

	/**
	 * @brief Updates the swapchains extent, if a swapchain exists
	 * @param extent The width and height of the new swapchain images
	 */
	void update_swapchain(const VkExtent2D &extent);

	/**
	 * @brief Updates the swapchains image count, if a swapchain exists
	 * @param image_count The amount of images in the new swapchain
	 */
	void update_swapchain(const uint32_t image_count);

	/**
	 * @brief Updates the swapchains image usage, if a swapchain exists
	 * @param image_usage_flags The usage flags the new swapchain images will have
	 */
	void update_swapchain(const std::set<VkImageUsageFlagBits> &image_usage_flags);

	/**
	 * @brief Updates the swapchains extent and surface transform, if a swapchain exists
	 * @param extent The width and height of the new swapchain images
	 * @param transform The surface transform flags
	 */
	void update_swapchain(const VkExtent2D &extent, const VkSurfaceTransformFlagBitsKHR transform);

	/**
	 * @brief Recreates the RenderFrames, called after every update
	 */
	void recreate();

	/**
	 * @brief begin_frame
	 *
	 * @return VkSemaphore
	 */
	VkSemaphore begin_frame();

	VkSemaphore submit(const Queue &queue, const CommandBuffer &command_buffer, VkSemaphore wait_semaphore, VkPipelineStageFlags wait_pipeline_stage);

	/**
	 * @brief Submits a command buffer related to a frame to a queue
	 */
	void submit(const Queue &queue, const CommandBuffer &command_buffer);

	/**
	 * @brief Waits a frame to finish its rendering
	 */
	void wait_frame();

	void end_frame(VkSemaphore semaphore);

	/**
	 * @brief An error should be raised if the frame is not active.
	 *        A frame is active after @ref begin_frame has been called.
	 * @return The current active frame
	 */
	RenderFrame &get_active_frame();

	/**
	 * @brief An error should be raised if a frame is active.
	 *        A frame is active after @ref begin_frame has been called.
	 * @return The previous frame
	 */
	RenderFrame &get_last_rendered_frame();

	/**
	 * @brief Requests a command buffer to the command pool of the active frame
	 *        A frame should be active at the moment of requesting it
	 * @param queue The queue command buffers will be submitted on
	 * @param reset_mode Indicate how the command buffer will be used, may trigger a
	 *        pool re-creation to set necessary flags
	 * @param level Command buffer level, either primary or secondary
	 * @param pool_index Select the frame command pool to use to manage the buffer
	 * @return A command buffer related to the current active frame
	 */
	CommandBuffer &request_frame_command_buffer(const Queue &            queue,
	                                            CommandBuffer::ResetMode reset_mode = CommandBuffer::ResetMode::ResetPool,
	                                            VkCommandBufferLevel     level      = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
	                                            uint16_t                 pool_index = 0);

	VkSemaphore request_semaphore();

	Device &get_device();

	void update_swapchain(std::unique_ptr<Swapchain> &&new_swapchain);

	Swapchain &get_swapchain();

	VkExtent2D get_surface_extent() const;

	uint32_t get_active_frame_index() const;

	std::vector<RenderFrame> &get_render_frames();

	virtual void handle_surface_changes();

  protected:
	VkExtent2D surface_extent;

  private:
	Device &device;

	std::unique_ptr<Swapchain> swapchain;

	SwapchainProperties swapchain_properties;

	// A list of present modes in order of priority (vector[0] has high priority, vector[size-1] has low priority)
	std::vector<VkPresentModeKHR> present_mode_priority_list = {
	    VK_PRESENT_MODE_FIFO_KHR,
	    VK_PRESENT_MODE_MAILBOX_KHR};

	// A list of surface formats in order of priority (vector[0] has high priority, vector[size-1] has low priority)
	std::vector<VkSurfaceFormatKHR> surface_format_priority_list = {
	    {VK_FORMAT_R8G8B8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR},
	    {VK_FORMAT_B8G8R8A8_SRGB, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR},
	    {VK_FORMAT_R8G8B8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR},
	    {VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR}};

	/// Current active frame index
	uint32_t active_frame_index{0};

	/// Whether a frame is active or not
	bool frame_active{false};

	std::vector<RenderFrame> frames;

	/// Queue to submit commands for rendering our frames
	const Queue &present_queue;

	RenderTarget::CreateFunc create_render_target = RenderTarget::DEFAULT_CREATE_FUNC;

	uint16_t command_pools_per_frame = 1;
};

}        // namespace vkb
