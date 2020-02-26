/*******************************************************************************
 * CGoGN: Combinatorial and Geometric modeling with Generic N-dimensional Maps  *
 * Copyright (C), IGG Group, ICube, University of Strasbourg, France            *
 *                                                                              *
 * This library is free software; you can redistribute it and/or modify it      *
 * under the terms of the GNU Lesser General Public License as published by the *
 * Free Software Foundation; either version 2.1 of the License, or (at your     *
 * option) any later version.                                                   *
 *                                                                              *
 * This library is distributed in the hope that it will be useful, but WITHOUT  *
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or        *
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License  *
 * for more details.                                                            *
 *                                                                              *
 * You should have received a copy of the GNU Lesser General Public License     *
 * along with this library; if not, write to the Free Software Foundation,      *
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA.           *
 *                                                                              *
 * Web site: http://cgogn.unistra.fr/                                           *
 * Contact information: cgogn@unistra.fr                                        *
 *                                                                              *
 *******************************************************************************/

#include <cgogn/rendering/shaders/compute_normals.h>

namespace cgogn
{

namespace rendering
{

static const char* vertex_shader_source1 =
	R"(
		#version 330
//		uniform usamplerBuffer tri_ind;
//		uniform samplerBuffer pos_vertex;
//		uniform float inv_h;
		out vec3 N;
		void main()
		{
				vec2 p = vec2(gl_VertexID%2, gl_VertexID/2);
				N = vec3(p,0);
				p = 2.0*p - 1.0;
				gl_Position = vec4(p,0,0);


//			vec2 d = vec2(1.0/16.0,inv_h);
//			int vid = gl_VertexID;
//			int ind_a = int(texelFetch(tri_ind, 3*gl_InstanceID+vid).r);
//			vec3 A = texelFetch(pos_vertex, ind_a).rgb;
//			vid  = (vid+1)%3;
//			int ind_b = int(texelFetch(tri_ind, 3*gl_InstanceID+vid).r);
//			vec3 B = texelFetch(pos_vertex, (ind_b)).rgb;
//			vid  = (vid+1)%3;
//			int ind_c = int(texelFetch(tri_ind, 3*gl_InstanceID+vid).r);
//			vec3 C = texelFetch(pos_vertex, ind_c).rgb;
//			N = cross(C-A,B-A);
//			uint ind_f = uint(ind_a%2);
//			vec2 coord_N = (-1.0+d) + 2.0 * d * vec2(float(ind_f%16u),float(ind_f/16u));
//			gl_Position = vec4(vec2(-0.9999,-0.9999)+0.00001*coord_N,0,1);
		}
		)";

static const char* fragment_shader_source1 =
	R"(
		#version 330
		out vec3 frag_out;
		in vec3 N;
		void main()
		{
			frag_out = N;
		}
		)";

ShaderComputeNormal1* ShaderComputeNormal1::instance_ = nullptr;

ShaderComputeNormal1::ShaderComputeNormal1()
{
	load2_bind(vertex_shader_source1, fragment_shader_source1);
	//	add_uniforms("tri_ind", "pos_vertex", "inv_h");
}

void ShaderParamComputeNormal1::set_uniforms()
{
	//	shader_->set_uniforms_values(10, vbo_pos_->bind_tb(11), 1.0f / float(height_tex_));
}

static const char* vertex_shader_source2 =
	R"(
		#version 330
		uniform sampler2D tex_normals;
		out vec3 vbo_out;
		const int w = 16;
		void main()
		{
			ivec2 icoord = ivec2(gl_VertexID%w,gl_VertexID/w);
			vec3 N = texelFetch(tex_normals,icoord,0).rgb;
			vbo_out = N;
//		vbo_out = normalize(N);
		}
		)";

ShaderComputeNormal2* ShaderComputeNormal2::instance_ = nullptr;

ShaderComputeNormal2::ShaderComputeNormal2()
{
	load_tfb1_bind(vertex_shader_source2, {"vbo_out"});
	add_uniforms("tex_normals");
}
void ShaderParamComputeNormal2::set_uniforms()
{
	shader_->set_uniforms_values(tex_->bind(0));
}

ComputeNormalEngine* ComputeNormalEngine::instance_ = nullptr;

ComputeNormalEngine::ComputeNormalEngine()
{
	param1_ = ShaderComputeNormal1::generate_param();
	param2_ = ShaderComputeNormal2::generate_param();
	param2_->tex_ = new Texture2D();
	param2_->tex_->alloc(0, 0, GL_RGBA32F, GL_RGBA, nullptr, GL_FLOAT);
	fbo_ = new FBO(std::vector<Texture2D*>{param2_->tex_}, false, nullptr);
	tfb_ = new TFB_ComputeNormal(*(param2_.get()));
}

ComputeNormalEngine::~ComputeNormalEngine()
{
	delete param2_->tex_;
	delete tfb_;
	delete fbo_;
}

void ComputeNormalEngine::compute(VBO* pos, MeshRender* renderer, VBO* normals)
{
	int32 h = (normals->size() + 15) / 16;
	//	fbo_->resize(16, h);
	fbo_->resize(64, 64);
	fbo_->bind();
	glDisable(GL_DEPTH_TEST);
	glClearColor(0.9, 0.9, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT);
	//	glEnable(GL_BLEND);
	//	glBlendEquation(GL_FUNC_ADD);
	//	glBlendFunc(GL_ONE, GL_ONE);
	param1_->vbo_pos_ = pos;
	param1_->height_tex_ = h;
	param1_->bind();
	glPointSize(1.00f);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	//	EBO* ebo = renderer->get_EBO(TRIANGLES);
	//	ebo->bind_tb(10);
	//	glPointSize(1.001f);
	//	glDrawArraysInstanced(GL_POINTS, 0, 3, int32(ebo->size()) / 3);
	//	ebo->release_tb();
	param1_->release();
	glDisable(GL_BLEND);
	fbo_->release();

	glClearColor(0, 0, 0, 0);
	tfb_->start(GL_POINTS, {normals});
	glDrawArrays(GL_POINTS, 0, normals->size());
	tfb_->stop();
	glEnable(GL_DEPTH_TEST);

	std::cout << *normals << std::endl;
}

} // namespace rendering

} // namespace cgogn
