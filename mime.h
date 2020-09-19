#define MIME_AMOUNT 10

#define MIME_HTML         0
#define MIME_CSS          1
#define MIME_JS           2
// data transfer
#define MIME_GZIP         3
#define MIME_JSON         4
// images
#define MIME_JPG          5
#define MIME_GIF          6
#define MIME_PNG          7
// sound
#define MIME_MP3          8
#define MIME_WAV          9


const char *mime_extension[MIME_AMOUNT];
const char *mime_type[MIME_AMOUNT];

const char* mime_get_type(const char *path);
