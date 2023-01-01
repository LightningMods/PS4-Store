#ifdef GL_ES
precision mediump float;
#endif

// glslsandbox uniforms
uniform float time;
uniform vec2 mouse;
uniform vec2 resolution;

// shadertoy emulation
#define iTime time
#define iMouse mouse
#define iResolution resolution

#define LAYERS 12
#define DEPTH .4
#define WIDTH .2
#define SPEED .5

vec2 mod289(vec2 x) {
  return x - floor(x * (1.0 / 289.0)) * 289.0;
}

vec3 mod289(vec3 x) {
  return x - floor(x * (1.0 / 289.0)) * 289.0;
}

vec4 mod289(vec4 x) {
  return x - floor(x * (1.0 / 289.0)) * 289.0;
}

vec3 permute(vec3 x) {
  return mod289(((x * 34.0) + 1.0) * x);
}

vec4 permute(vec4 x) {
  return mod((34.0 * x + 1.0) * x, 289.0);
}

vec4 taylorInvSqrt(vec4 r) {
  return 1.79284291400159 - 0.85373472095314 * r;
}

float snoise(vec2 v) {
  const vec4 C = vec4(0.211324865405187, 0.366025403784439, -0.577350269189626, 0.024390243902439);
  vec2 i = floor(v + dot(v, C.yy));
  vec2 x0 = v - i + dot(i, C.xx);

  vec2 i1;
  i1 = (x0.x > x0.y) ? vec2(1.0, 0.0) : vec2(0.0, 1.0);
  vec4 x12 = x0.xyxy + C.xxzz;
  x12.xy -= i1;

  i = mod289(i); // Avoid truncation effects in permutation
  vec3 p = permute(permute(i.y + vec3(0.0, i1.y, 1.0)) +
    i.x + vec3(0.0, i1.x, 1.0));

  vec3 m = max(0.5 - vec3(dot(x0, x0), dot(x12.xy, x12.xy), dot(x12.zw, x12.zw)), 0.0);
  m = m * m;
  m = m * m;

  vec3 x = 2.0 * fract(p * C.www) - 1.0;
  vec3 h = abs(x) - 0.5;
  vec3 ox = floor(x + 0.5);
  vec3 a0 = x - ox;

  m *= 1.79284291400159 - 0.85373472095314 * (a0 * a0 + h * h);

  vec3 g;
  g.x = a0.x * x0.x + h.x * x0.y;
  g.yz = a0.yz * x12.xz + h.yz * x12.yw;

  return 130.0 * dot(m, g);
}

float cellular2x2(vec2 P) {
  #define K 0.142857142857 // 1/7
  #define K2 0.0714285714285 // K/2
  #define jitter 0.8 // jitter 1.0 makes F1 wrong more often

  vec2 Pi = mod(floor(P), 289.0);
  vec2 Pf = fract(P);
  vec4 Pfx = Pf.x + vec4(-0.5, -1.5, -0.5, -1.5);
  vec4 Pfy = Pf.y + vec4(-0.5, -0.5, -1.5, -1.5);
  vec4 p = permute(Pi.x + vec4(0.0, 1.0, 0.0, 1.0));
  p = permute(p + Pi.y + vec4(0.0, 0.0, 1.0, 1.0));
  vec4 ox = mod(p, 7.0) * K + K2;
  vec4 oy = mod(floor(p * K), 7.0) * K + K2;
  vec4 dx = Pfx + jitter * ox;
  vec4 dy = Pfy + jitter * oy;
  vec4 d = dx * dx + dy * dy; // d11, d12, d21 and d22, squared
  // Sort out the two smallest distances

  // Cheat and pick only F1
  d.xy = min(d.xy, d.zw);
  d.x = min(d.x, d.y);
  return d.x; // F1 duplicated, F2 not computed
}

float fbm(vec2 p) {
  float f = 0.0;
  float w = 0.5;
  for (int i = 0; i < 5; i++) {
    f += w * snoise(p);
    p *= 2.;
    w *= 0.5;
  }
  return f;
}

void main(void) {
  const mat3 p = mat3(13.323122, 23.5112, 21.71123, 21.1212, 28.7312, 11.9312, 21.8112, 14.7212, 61.3934);
  vec2 uv = gl_FragCoord.xy / iResolution.y;
  vec3 acc = vec3(0.0);
  float dof = 5. * sin(iTime * .1);
  for (int i = 0; i < LAYERS; i++) {
    float fi = float(i);
    vec2 q = uv * (1. + fi * DEPTH);
    q += vec2(q.y * (WIDTH * mod(fi * 7.238917, 1.) - WIDTH * .5), SPEED * iTime / (1. + fi * DEPTH * .03));
    vec3 n = vec3(floor(q), 31.189 + fi);
    vec3 m = floor(n) * .00001 + fract(n);
    vec3 mp = (31415.9 + m) / fract(p * m);
    vec3 r = fract(mp);
    vec2 s = abs(mod(q, 1.) - .5 + .9 * r.xy - .45);
    s += .01 * abs(2. * fract(10. * q.yx) - 1.);
    float d = .6 * max(s.x - s.y, s.x + s.y) + max(s.x, s.y) - .01;
    float edge = .005 + .05 * min(.5 * abs(fi - 5. - dof), 1.);
    acc += vec3(smoothstep(edge, -edge, d) * (r.x / (1. + .02 * fi * DEPTH)));
  }

  uv.x *= (iResolution.x / iResolution.y);

  vec2 suncent = vec2(0.3, 0.9);

  float suns = (1.0 - distance(uv, suncent));
  suns = clamp(0.2 + suns, 0.0, 1.0);
  float sunsh = smoothstep(0.85, 0.95, suns);

  float slope;
  slope = 0.8 + uv.x - (uv.y * 2.3);
  slope = 1.0 - smoothstep(0.55, 0.0, slope);

  float noise = abs(fbm(uv * 1.5));
  slope = (noise * 0.2) + (slope - ((1.0 - noise) * slope * 0.1)) * 0.6;
  slope = clamp(slope, 0.0, 1.0);

  vec2 GA;
  GA.x -= iTime * 1.8;
  GA.y += iTime * 0.9;
  GA *= 0.0;

  float Snowout = 0.0;

  Snowout = 0.35 + (slope * (suns + 0.3)) + (sunsh * 0.6);
  gl_FragColor = vec4(Snowout * 0.9, Snowout, Snowout * 1.1, 1.0);
  gl_FragColor += vec4(vec3(acc), 1.0);
}