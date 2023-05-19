#pragma once
#include <curl/curl.h>
#include <memory>
#include "GLES2_common.h"
typedef enum token_name
{
	ID,
	NAME,
	DESC,
	IMAGE,
	PACKAGE,
	VERSION,
	PICPATH,
	DESC_1,
	DESC_2,
	REVIEWSTARS,
	SIZE,
	AUTHOR,
	APPTYPE,
	PV,
	MAIN_ICON_PATH,
	MAIN_MENU_PIC,
	RELEASEDATE,
	NUMBER_OF_DOWNLOADS,
	GITHUB_LINK,
	VIDEO_LINK,
	TWITTER_LINK,
	PKG_MD5_HASH,
	TOTAL_NUM_OF_TOKENS,
	// should match NUM_OF_USER_TOKENS
} token_name;

static const char* used_token[] =
{
	"id",
	"name",
	"desc",
	"image",
	"package",
	"version",
	"picpath",
	"desc_1",
	"desc_2",
	"ReviewStars",
	"Size",
	"Author",
	"apptype",
	"pv",
	"main_icon_path",
	"main_menu_pic",
	"releaseddate",
	"number_of_downloads",
	"github",
	"video",
	"twitter",
	"md5",
};

#define NUM_OF_USER_TOKENS  (sizeof(used_token) / sizeof(used_token[0]))
#include <string>
#include <vector>


struct dl_arg_t {
    std::string url, dst;
    std::atomic<int> req, idx, connid, tmpid, status, g_idx;
    CURL* curl_handle;
    curl_off_t last_off;
    FILE* dlfd;
    std::atomic<double> progress;
    std::atomic<uint64_t> contentLength;
    std::vector<item_idx_t> token_d;
    std::atomic<bool> is_threaded;
	bool paused = false;
    bool running = true;

    dl_arg_t()
        : req(-1),
          idx(-1),
          connid(-1),
          tmpid(-1),
          status(-1),
          g_idx(-1),
          curl_handle(NULL),
          last_off(0),
          dlfd(NULL),
          progress(0.0),
          contentLength(0),
          is_threaded(false) {}
};

int dl_from_url_v2(std::string url_, std::string dst_, std::vector<item_idx_t>& t);
int dl_from_url_v3_threaded(std::string url_, std::string dst_, dl_arg_t *arg);
int dl_from_url(std::string url_, std::string dst_);
int ini_dl_req(dl_arg_t *i);