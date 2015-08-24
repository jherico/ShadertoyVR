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

namespace oria {

    void compileProgram(ProgramPtr & result, const std::string& vs, const std::string& fs) {
        using namespace oglplus;
        try {
            result.reset(new Program());
            // attach the shaders to the program
            result->AttachShader(
                VertexShader()
                .Source(GLSLSource(vs))
                .Compile()
                );
            result->AttachShader(
                FragmentShader()
                .Source(GLSLSource(fs))
                .Compile()
                );
            result->Link();
        } catch (ProgramBuildError & err) {
            qWarning() << (const char*)err.Message;
            result.reset();
        }
    }

    ProgramPtr loadProgram(const QString& vsFile, const QString& fsFile) {
        ProgramPtr result;
        compileProgram(result,
            readFileToString(vsFile).toUtf8().data(),
            readFileToString(fsFile).toUtf8().data());
        return result;
    }

    UniformMap getActiveUniforms(ProgramPtr & program) {
        UniformMap activeUniforms;
        size_t uniformCount = program->ActiveUniforms().Size();
        for (size_t i = 0; i < uniformCount; ++i) {
            std::string name = program->ActiveUniforms().At(i).Name();
            activeUniforms[name] = program->ActiveUniforms().At(i).Index();
        }
        return activeUniforms;
    }

}
