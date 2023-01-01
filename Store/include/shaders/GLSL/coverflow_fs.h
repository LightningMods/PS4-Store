

static const char coverflow_fs0[] =
       "precision mediump float;  \
        varying   vec4 fragColor; \
        varying   vec3 norm; \
        \
        void main() \
        { \
                gl_FragColor = fragColor; \
        }";


static const char coverflow_fs1[] =
       "precision mediump float; \
        uniform sampler2D texture; \
        \
        varying vec2 vTexCoord; \
        varying vec4 fragColor; \
        \
        void main() \
        { \
            gl_FragColor    = texture2D(texture, vTexCoord); \
            gl_FragColor.a *= fragColor.a; \
        }";

static const char coverflow_fs2[] =
       "precision mediump float; \
        uniform sampler2D texture; \
        \
        varying vec2 vTexCoord; \
        varying vec4 fragColor; \
        \
        void main() \
        { \
            gl_FragColor   = texture2D(texture, vTexCoord); \
            gl_FragColor.a = sin(vTexCoord.y) * fragColor.a; \
}";

static const int coverflow_fs0_length = 0;
static const int coverflow_fs1_length = 0;
static const int coverflow_fs2_length = 0;