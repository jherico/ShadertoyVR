#include "Common.h"
#include "Shadertoy.h"

const char * st::UNIFORM_RESOLUTION = "iResolution";
const char * st::UNIFORM_GLOBALTIME = "iGlobalTime";
const char * st::UNIFORM_CHANNEL_TIME = "iChannelTime";
const char * st::UNIFORM_MOUSE_COORDS = "iMouse";
const char * st::UNIFORM_DATE = "iDate";
const char * st::UNIFORM_SAMPLE_RATE = "iSampleRate";
const char * st::UNIFORM_POSITION = "iPos";
const char * st::UNIFORM_CHANNEL_RESOLUTIONS[4] = {
  "iChannelResolution[0]",
  "iChannelResolution[1]",
  "iChannelResolution[2]",
  "iChannelResolution[3]",
};
const char * st::UNIFORM_CHANNELS[4] = {
  "iChannel0",
  "iChannel1",
  "iChannel2",
  "iChannel3",
};

const char * st::SHADER_HEADER = "#version 330\n"
  "uniform vec3      iResolution;           // viewport resolution (in pixels)\n"
  "uniform float     iGlobalTime;           // shader playback time (in seconds)\n"
  "uniform float     iChannelTime[4];       // channel playback time (in seconds)\n"
  "uniform vec3      iChannelResolution[4]; // channel resolution (in pixels)\n"
  "uniform vec4      iMouse;                // mouse pixel coords. xy: current (if MLB down), zw: click\n"
  "uniform vec4      iDate;                 // (year, month, day, time in seconds)\n"
  "uniform float     iSampleRate;           // sound sample rate (i.e., 44100)\n"
  "uniform vec3      iPos; // Head position\n"
  "in vec3 iDir; // Direction from viewer\n"
  "out vec4 FragColor;\n";

const char * st::LINE_NUMBER_HEADER =
  "#line 1\n";

const QStringList st::TEXTURES({
  "/presets/tex00.jpg",
  "/presets/tex01.jpg",
  "/presets/tex02.jpg",
  "/presets/tex03.jpg",
  "/presets/tex04.jpg",
  "/presets/tex05.jpg",
  "/presets/tex06.jpg",
  "/presets/tex07.jpg",
  "/presets/tex08.jpg",
  "/presets/tex09.jpg",
  "/presets/tex10.png",
  "/presets/tex11.png",
  "/presets/tex12.png",
  "/presets/tex14.png",
  "/presets/tex15.png",
  "/presets/tex16.png"
});

const QStringList st::CUBEMAPS({
  "/presets/cube00_%1.jpg",
  "/presets/cube01_%1.png",
  "/presets/cube02_%1.jpg",
  "/presets/cube03_%1.png",
  "/presets/cube04_%1.png",
  "/presets/cube05_%1.png",
});
