/************************************************************************************

 Authors     :   Bradley Austin Davis <bdavis@saintandreas.org>
 Copyright   :   Copyright Bradley Austin Davis. All Rights reserved.

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
#include "QtCommon.h"

namespace qt {
	QSize sizeFromGlm(const vec2 & size) {
		return QSize(size.x, size.y);
	}

	QPointF pointFromGlm(const vec2 & pt) {
		return QPointF(pt.x, pt.y);
	}
}

QJsonValue path(const QJsonValue & parent, std::initializer_list<QVariant> elements) {
	QJsonValue current = parent;
	std::for_each(elements.begin(), elements.end(), [&](const QVariant & element) {
		if (current.isObject()) {
			QString path = element.toString();
			current = current.toObject().value(path);
		}
		else if (current.isArray()) {
			int offset = element.toInt();
			current = current.toArray().at(offset);
		}
		else {
			qWarning() << "Unable to continue";
			current = QJsonValue();
		}
	});
	return current;
}

typedef std::list<QString> List;
typedef std::map<QString, List> Map;
typedef std::pair<QString, List> Pair;

template <typename F>
void for_each_node(const QDomNodeList & list, F f) {
	for (int i = 0; i < list.size(); ++i) {
		f(list.at(i));
	}
}

static ImagePtr loadImageWithAlpha(const std::vector<uint8_t> & data, bool flip) {
	using namespace oglplus;
	return ImagePtr();
}

QSurfaceFormat getDesiredSurfaceFormat() {
	QSurfaceFormat format;
	format.setDepthBufferSize(16);
	format.setStencilBufferSize(8);
	format.setMajorVersion(4);
	format.setMinorVersion(3);
	format.setProfile(QSurfaceFormat::OpenGLContextProfile::CoreProfile);
#ifdef DEBUG
	format.setOption(QSurfaceFormat::DebugContext);
#endif
	return format;
}
