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

#include "common/error.h"

VKBP_DISABLE_WARNINGS()
#include <glm/glm.hpp>
VKBP_ENABLE_WARNINGS()

#include "rendering/subpass.h"

namespace vkb
{
namespace sg
{
class Scene;
class Node;
class Mesh;
class SubMesh;
class Camera;
}        // namespace sg

/**
 * @brief Global uniform structure for base shader
 */
struct alignas(16) GlobalUniform
{
	glm::mat4 model;

	glm::mat4 camera_view_proj;

	glm::vec4 light_pos;

	glm::vec4 light_color;
};

/**
 * @brief PBR material uniform for base shader
 */
struct PBRMaterialUniform
{
	glm::vec4 base_color_factor;

	float metallic_factor;

	float roughness_factor;
};

/**
 * @brief This subpass is responsible for rendering a Scene
 */
class SceneSubpass : public Subpass
{
  public:
	/**
	 * @brief Constructs a subpass
	 * @param render_context Render context
	 * @param vertex_shader Vertex shader source
	 * @param fragment_shader Fragment shader source
	 * @param scene Scene to render on this subpass
	 * @param camera Camera used to look at the scene
	 */
	SceneSubpass(RenderContext &render_context, ShaderSource &&vertex_shader, ShaderSource &&fragment_shader, sg::Scene &scene, sg::Camera &camera);

	virtual ~SceneSubpass() = default;

	/**
	 * @brief Record draw commands
	 */
	virtual void draw(CommandBuffer &command_buffer) override;

	void update_uniform(CommandBuffer &command_buffer, sg::Node &node);

	void draw_submesh(CommandBuffer &command_buffer, sg::SubMesh &sub_mesh);

  protected:
	/**
	 * @brief Sorts objects based on distance from camera and classifies them
	 *        into opaque and transparent in the arrays provided
	 */
	void get_sorted_nodes(std::multimap<float, std::pair<sg::Node *, sg::SubMesh *>> &opaque_nodes,
	                      std::multimap<float, std::pair<sg::Node *, sg::SubMesh *>> &transparent_nodes);

  private:
	void draw_submesh_command(CommandBuffer &command_buffer, sg::SubMesh &sub_mesh);

	sg::Camera &camera;

	std::vector<sg::Mesh *> meshes;

	GlobalUniform global_uniform;
};

}        // namespace vkb
