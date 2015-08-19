/************************************************************************************

 Authors     :   Bradley Austin Davis <bdavis@saintandreas.org>
 Copyright   :   Copyright Brad Davis. All Rights reserved.

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.

 ************************************************************************************/

#include "Common.h"

#pragma warning( disable : 4068 4244 4267 4065 4101 4244)
#include <oglplus/bound/buffer.hpp>
#include <oglplus/shapes/cube.hpp>
#include <oglplus/shapes/sphere.hpp>
#include <oglplus/images/image.hpp>
#include <oglplus/shapes/wicker_torus.hpp>
#include <oglplus/shapes/sky_box.hpp>
#include <oglplus/shapes/plane.hpp>
#include <oglplus/shapes/grid.hpp>
#include <oglplus/shapes/vector.hpp>
#include <oglplus/opt/list_init.hpp>
#include <oglplus/shapes/obj_mesh.hpp>
#pragma warning( default : 4068 4244 4267 4065 4101)

namespace oria {

	template <typename Iter>
	void renderGeometryWithLambdas(ShapeWrapperPtr & shape, ProgramPtr & program, Iter begin, const Iter & end) {
		program->Use();

		Mat4Uniform(*program, "ModelView").Set(Stacks::modelview().top());
		Mat4Uniform(*program, "Projection").Set(Stacks::projection().top());

		std::for_each(begin, end, [&](const std::function<void()>&f) {
			f();
		});

		shape->Use();
		shape->Draw();

		oglplus::NoProgram().Bind();
		oglplus::NoVertexArray().Bind();
	}

	void renderGeometry(ShapeWrapperPtr & shape, ProgramPtr & program, std::function<void()> lambda) {
		renderGeometry(shape, program, LambdaList({ lambda }));
	}

	void renderGeometry(ShapeWrapperPtr & shape, ProgramPtr & program, const std::list<std::function<void()>> & list) {
		renderGeometryWithLambdas(shape, program, list.begin(), list.end());
	}

	void renderGeometry(ShapeWrapperPtr & shape, ProgramPtr & program) {
		static const std::list<std::function<void()>> EMPTY_LIST;
		renderGeometryWithLambdas(shape, program, EMPTY_LIST.begin(), EMPTY_LIST.end());
	}


	void renderCube(const glm::vec3 & color) {
		using namespace oglplus;

		static ProgramPtr program;
		static ShapeWrapperPtr shape;
		if (!program) {
			program = loadProgram(Resource::SHADERS_SIMPLE_VS, Resource::SHADERS_COLORED_FS);
			shape = ShapeWrapperPtr(new shapes::ShapeWrapper(List("Position").Get(), shapes::Cube(), *program));
			Platform::addShutdownHook([&] {
				program.reset();
				shape.reset();
			});
		}
		program->Use();
		Uniform<vec4>(*program, "Color").Set(vec4(color, 1));
		renderGeometry(shape, program);
	}

	void renderColorCube() {
		using namespace oglplus;

		static ProgramPtr program;
		static ShapeWrapperPtr shape;
		if (!program) {
			program = loadProgram(Resource::SHADERS_COLORCUBE_VS, Resource::SHADERS_COLORCUBE_FS);
			shape = ShapeWrapperPtr(new shapes::ShapeWrapper(List("Position")("Normal").Get(), shapes::Cube(), *program));;
			Platform::addShutdownHook([&] {
				program.reset();
				shape.reset();
			});
		}

		renderGeometry(shape, program);
	}

	ShapeWrapperPtr loadSkybox(ProgramPtr program) {
		using namespace oglplus;
		ShapeWrapperPtr shape = ShapeWrapperPtr(new shapes::ShapeWrapper(List("Position").Get(), shapes::SkyBox(), *program));
		return shape;
	}


	ShapeWrapperPtr loadPlane(ProgramPtr program, float aspect) {
		using namespace oglplus;
		Vec3f a(1, 0, 0);
		Vec3f b(0, 1, 0);
		if (aspect > 1) {
			b[1] /= aspect;
		}
		else {
			a[0] *= aspect;
		}
		return ShapeWrapperPtr(
			new shapes::ShapeWrapper(
		{ "Position", "TexCoord" },
				shapes::Plane(a, b),
				*program
				)
			);
	}

	void renderSkybox(Resource firstImageResource) {
		using namespace oglplus;

		static ProgramPtr program;
		static ShapeWrapperPtr shape;
		if (!program) {
			program = loadProgram(Resource::SHADERS_CUBEMAP_VS, Resource::SHADERS_CUBEMAP_FS);
			shape = ShapeWrapperPtr(new shapes::ShapeWrapper(List("Position").Get(), shapes::SkyBox(), *program));
			Platform::addShutdownHook([&] {
				program.reset();
				shape.reset();
			});
		}

		TexturePtr texture = loadCubemapTexture(firstImageResource);
		texture->Bind(TextureTarget::CubeMap);
		MatrixStack & mv = Stacks::modelview();
		mv.withPush([&] {
			Context::Disable(Capability::DepthTest);
			Context::Disable(Capability::CullFace);
			renderGeometry(shape, program);
			Context::Enable(Capability::CullFace);
			Context::Enable(Capability::DepthTest);
		});
		DefaultTexture().Bind(TextureTarget::CubeMap);
	}

	void renderFloor() {
		using namespace oglplus;
		const float SIZE = 100;
		static ProgramPtr program;
		static ShapeWrapperPtr shape;
		static TexturePtr texture;
		if (!program) {
			program = loadProgram(Resource::SHADERS_TEXTURED_VS, Resource::SHADERS_TEXTURED_FS);
			shape = ShapeWrapperPtr(new shapes::ShapeWrapper(List("Position")("TexCoord").Get(), shapes::Plane(), *program));
			texture = load2dTexture(Resource::IMAGES_FLOOR_PNG);
			Context::Bound(TextureTarget::_2D, *texture).MinFilter(TextureMinFilter::LinearMipmapNearest).GenerateMipmap();
			Platform::addShutdownHook([&] {
				program.reset();
				shape.reset();
				texture.reset();
			});
		}

		texture->Bind(TextureTarget::_2D);
		MatrixStack & mv = Stacks::modelview();
		mv.withPush([&] {
			mv.scale(vec3(SIZE));
			renderGeometry(shape, program, [&] {
				oglplus::Uniform<vec2>(*program, "UvMultiplier").Set(vec2(SIZE * 2.0f));
			});
		});

		DefaultTexture().Bind(TextureTarget::_2D);
	}

	ShapeWrapperPtr loadShape(const std::initializer_list<const GLchar*>& names, Resource resource) {
		using namespace oglplus;
		return ShapeWrapperPtr(new shapes::ShapeWrapper(names, shapes::CtmMesh(resource)));
	}

	ShapeWrapperPtr loadShape(const std::initializer_list<const GLchar*>& names, Resource resource, ProgramPtr program) {
		using namespace oglplus;
		return ShapeWrapperPtr(new shapes::ShapeWrapper(names, shapes::CtmMesh(resource), *program));
	}

	void renderManikin() {
		static ProgramPtr program;
		static ShapeWrapperPtr shape;

		if (!program) {
			program = loadProgram(Resource::SHADERS_LIT_VS, Resource::SHADERS_LITCOLORED_FS);
			shape = loadShape({ "Position", "Normal" }, Resource::MESHES_MANIKIN_CTM, program);
			Platform::addShutdownHook([&] {
				program.reset();
				shape.reset();
			});
		}


		renderGeometry(shape, program, [&] {
			bindLights(program);
		});
	}

	void renderRift() {
		using namespace oglplus;
		static ProgramPtr program;
		static ShapeWrapperPtr shape;
		if (!program) {
			Platform::addShutdownHook([&] {
				program.reset();
				shape.reset();
			});

			program = loadProgram(Resource::SHADERS_LIT_VS, Resource::SHADERS_LITCOLORED_FS);
			shape = ::oria::loadShape({ "Position", "Normal" }, Resource::MESHES_RIFT_CTM, program);
		}

		auto & mv = Stacks::modelview();
		mv.withPush([&] {
			mv.rotate(-QUARTER_TAU - 0.22f, Vectors::X_AXIS).scale(0.5f);
			renderGeometry(shape, program, [&] {
				oria::bindLights(program);
			});
		});
	}

	void renderArtificialHorizon(float alpha) {
		using namespace oglplus;
		static ProgramPtr program;
		static ShapeWrapperPtr shape;
		static std::vector<Vec4f> materials = {
			Vec4f(0.351366f, 0.665379f, 0.800000f, 1),
			Vec4f(0.640000f, 0.179600f, 0.000000f, 1),
			Vec4f(0.000000f, 0.000000f, 0.000000f, 1),
			Vec4f(0.171229f, 0.171229f, 0.171229f, 1),
			Vec4f(0.640000f, 0.640000f, 0.640000f, 1)
		};

		if (!program) {
			Platform::addShutdownHook([&] {
				program.reset();
				shape.reset();
			});

			program = loadProgram(Resource::SHADERS_LITMATERIALS_VS, Resource::SHADERS_LITCOLORED_FS);
			std::stringstream && stream = Platform::getResourceStream(Resource::MESHES_ARTIFICIAL_HORIZON_OBJ);
			shapes::ObjMesh mesh(stream);
			shape = ShapeWrapperPtr(new shapes::ShapeWrapper({ "Position", "Normal", "Material" }, mesh, *program));
			Uniform<Vec4f>(*program, "Materials[0]").Set(materials);
		}

		auto & mv = Stacks::modelview();
		mv.withPush([&] {
			renderGeometry(shape, program, [&] {
				Uniform<float>(*program, "ForceAlpha").Set(alpha);
				oria::bindLights(program);
			});
		});

	}

	void renderManikinScene(float ipd, float eyeHeight) {
		oria::renderSkybox(Resource::IMAGES_SKY_CITY_XNEG_PNG);
		oria::renderFloor();

		// Scale the size of the cube to the distance between the eyes
		MatrixStack & mv = Stacks::modelview();

		mv.withPush([&] {
			mv.translate(glm::vec3(0, eyeHeight, 0)).scale(glm::vec3(ipd));
			oria::renderColorCube();
		});

		mv.withPush([&] {
			mv.translate(glm::vec3(0, 0, ipd * -5.0));
			oglplus::Context::Disable(oglplus::Capability::CullFace);
			oria::renderManikin();
		});
	}

	void renderExampleScene(float ipd, float eyeHeight) {
		oria::renderSkybox(Resource::IMAGES_SKY_CITY_XNEG_PNG);
		oria::renderFloor();

		MatrixStack & mv = Stacks::modelview();
		mv.withPush([&] {
			mv.translate(glm::vec3(0, eyeHeight, 0)).scale(glm::vec3(ipd));
			oria::renderColorCube();
		});
		mv.withPush([&] {
			mv.translate(glm::vec3(0, eyeHeight / 2, 0)).scale(glm::vec3(ipd / 2, eyeHeight, ipd / 2));
			oria::renderColorCube();
		});

	}


	void GL_CALLBACK debugCallback(
		GLenum source,
		GLenum type,
		GLuint id,
		GLenum severity,
		GLsizei length,
		const GLchar * message,
		void * userParam) {
		const char * typeStr = "?";
		switch (type) {
		case GL_DEBUG_TYPE_ERROR:
			typeStr = "ERROR";
			break;
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
			typeStr = "DEPRECATED_BEHAVIOR";
			break;
		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
			typeStr = "UNDEFINED_BEHAVIOR";
			break;
		case GL_DEBUG_TYPE_PORTABILITY:
			typeStr = "PORTABILITY";
			break;
		case GL_DEBUG_TYPE_PERFORMANCE:
			typeStr = "PERFORMANCE";
			break;
		case GL_DEBUG_TYPE_OTHER:
			typeStr = "OTHER";
			break;
		}

		const char * severityStr = "?";
		switch (severity) {
		case GL_DEBUG_SEVERITY_LOW:
			severityStr = "LOW";
			break;
		case GL_DEBUG_SEVERITY_MEDIUM:
			severityStr = "MEDIUM";
			break;
		case GL_DEBUG_SEVERITY_HIGH:
			severityStr = "HIGH";
			break;
		}
		SAY("--- OpenGL Callback Message ---");
		SAY("type: %s\nseverity: %-8s\nid: %d\nmsg: %s", typeStr, severityStr, id,
			message);
		SAY("--- OpenGL Callback Message ---");
	}

	ShapeWrapperPtr loadSphere(const std::initializer_list<const GLchar*>& names, ProgramPtr program) {
		using namespace oglplus;
		return ShapeWrapperPtr(new shapes::ShapeWrapper(names, shapes::Sphere(), *program));
	}

	ShapeWrapperPtr loadGrid(ProgramPtr program) {
		using namespace oglplus;
		return ShapeWrapperPtr(new shapes::ShapeWrapper(std::initializer_list<const GLchar*>({ "Position" }), shapes::Grid(
			Vec3f(0.0f, 0.0f, 0.0f),
			Vec3f(1.0f, 0.0f, 0.0f),
			Vec3f(0.0f, 0.0f, -1.0f),
			8,
			8), *program));
	}

	void draw3dGrid() {
		static ProgramPtr program;
		static ShapeWrapperPtr grid;
		if (!program) {
			program = loadProgram(Resource::SHADERS_SIMPLE_VS, Resource::SHADERS_COLORED_FS);
			grid = loadGrid(program);
			Platform::addShutdownHook([&] {
				program.reset();
				grid.reset();
			});
		}
		renderGeometry(grid, program);
	}

	/*
	 For the sin of writing compat mode OpenGL, I will go to the special hell.
	*/
	void draw3dVector(const glm::vec3 & end, const glm::vec3 & col) {
		oglplus::NoProgram().Bind();
		glMatrixMode(GL_PROJECTION);
		glLoadMatrixf(glm::value_ptr(Stacks::projection().top()));
		glMatrixMode(GL_MODELVIEW);
		glLoadMatrixf(glm::value_ptr(Stacks::modelview().top()));
		glColor3f(col.r, col.g, col.b);
		glBegin(GL_LINES);
		glVertex3f(0, 0, 0);
		glVertex3f(end.x, end.y, end.z);
		glEnd();
		GLenum err = glGetError();
	}

}

