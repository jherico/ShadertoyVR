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

#pragma once

class Platform {
public:
  static void sleepMillis(int millis);
  static long elapsedMillis();
  static float elapsedSeconds();

  static void addShutdownHook(std::function<void()> f);
  static void runShutdownHooks();
};


inline QByteArray readFileToByteArray(const QString & fileName) {
    QFile f(fileName);
    f.open(QFile::ReadOnly);
    return f.readAll();
}

inline std::vector<uint8_t> readFileToVector(const QString & fileName) {
    QByteArray ba = readFileToByteArray(fileName);
    return std::vector<uint8_t>(ba.constData(), ba.constData() + ba.size());
}

inline QString readFileToString(const QString & fileName) {
    return QString(readFileToByteArray(fileName));
}
