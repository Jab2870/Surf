/* modifier 0 means no modifier */
static int surfuseragent    = 1;  /* Append Surf version to default WebKit user agent */
static char *fulluseragent  = ""; /* Or override the whole user agent string */
static char *scriptfile     = "~/.surf/script.js";
static char *styledir       = "~/.surf/styles/";
static char *certdir        = "~/.surf/certificates/";
static char *cachedir       = "~/.surf/cache/";
static char *cookiefile     = "~/.surf/cookies.txt";
static char *historyfile    = "~/.surf/history.txt";

/* Webkit default features */
/* Highest priority value will be used.
 * Default parameters are priority 0
 * Per-uri parameters are priority 1
 * Command parameters are priority 2
 */
static Parameter defconfig[ParameterLast] = {
	/* parameter                    Arg value       priority */
	[AcceleratedCanvas]   =       { { .i = 1 },     },
	[AccessMicrophone]    =       { { .i = 0 },     },
	[AccessWebcam]        =       { { .i = 0 },     },
	[Certificate]         =       { { .i = 0 },     },
	[CaretBrowsing]       =       { { .i = 0 },     },
	[CookiePolicies]      =       { { .v = "@Aa" }, },
	[DefaultCharset]      =       { { .v = "UTF-8" }, },
	[DiskCache]           =       { { .i = 1 },     },
	[DNSPrefetch]         =       { { .i = 0 },     },
	[FileURLsCrossAccess] =       { { .i = 0 },     },
	[FontSize]            =       { { .i = 12 },    },
	[FrameFlattening]     =       { { .i = 0 },     },
	[Geolocation]         =       { { .i = 0 },     },
	[HideBackground]      =       { { .i = 0 },     },
	[Inspector]           =       { { .i = 0 },     },
	[Java]                =       { { .i = 0 },     },
	[JavaScript]          =       { { .i = 0 },     },
	[KioskMode]           =       { { .i = 0 },     },
	[LoadImages]          =       { { .i = 1 },     },
	[MediaManualPlay]     =       { { .i = 1 },     },
	[Plugins]             =       { { .i = 1 },     },
	[PreferredLanguages]  =       { { .v = (char *[]){ NULL } }, },
	[RunInFullscreen]     =       { { .i = 0 },     },
	[ScrollBars]          =       { { .i = 0 },     },
	[ShowIndicators]      =       { { .i = 1 },     },
	[SiteQuirks]          =       { { .i = 1 },     },
	[SmoothScrolling]     =       { { .i = 0 },     },
	[SpellChecking]       =       { { .i = 1 },     },
	[SpellLanguages]      =       { { .v = ((char *[]){ "en_GB", NULL }) }, },
	[StrictTLS]           =       { { .i = 1 },     },
	[Style]               =       { { .i = 1 },     },
	[WebGL]               =       { { .i = 0 },     },
	[ZoomLevel]           =       { { .f = 1.2 },   },
};

static UriParameters uriparams[] = {
	{ "(://|\\.)suckless\\.org(/|$)", {
	  [JavaScript] = { { .i = 0 }, 1 },
	  [Plugins]    = { { .i = 0 }, 1 },
	}, },
	{ "(://|\\.)slack\\.com(/|$)", {
	  [JavaScript] = { { .i = 1 }, 1 },
	}, },
	{ "(://|\\.)duckduckgo\\.(com|co.uk)(/|$)", {
	  [JavaScript] = { { .i = 1 }, 1 },
	}, },
};

/* default window size: width, height */
static int winsize[] = { 800, 600 };

static WebKitFindOptions findopts = WEBKIT_FIND_OPTIONS_CASE_INSENSITIVE |
                                    WEBKIT_FIND_OPTIONS_WRAP_AROUND;

#define PROMPT_GO   "Go:"
#define PROMPT_FIND "Find:"

/* SETPROP(readprop, setprop, prompt)*/
#define SETPROP(r, s, p) { \
        .v = (const char *[]){ "/bin/sh", "-c", \
             "prop=\"$(printf '%b' \"$(xprop -id $1 $2 " \
             "| sed \"s/^$2(STRING) = //;s/^\\\"\\(.*\\)\\\"$/\\1/\")\" " \
             "| dmenu -p \"$4\" -w $1)\" && xprop -id $1 -f $3 8s -set $3 \"$prop\"", \
             "surf-setprop", winid, r, s, p, NULL \
        } \
}

/* DOWNLOAD(URI, referer) */
#define DOWNLOAD(u, r) { \
        .v = (const char *[]){ "st", "-e", "/bin/sh", "-c",\
             "download surf-download \"$1\" \"$2\" \"$3\" \"$4\"; read", \
             "surf-download", useragent, cookiefile, r, u, NULL \
        } \
}

/* PLUMB(URI) */
/* This called when some URI which does not begin with "about:",
 * "http://" or "https://" should be opened.
 */
#define PLUMB(u) {\
        .v = (const char *[]){ "/bin/sh", "-c", \
             "xdg-open \"$0\"", u, NULL \
        } \
}

/* VIDEOPLAY(URI) */
#define VIDEOPLAY(u) {\
        .v = (const char *[]){ "/bin/sh", "-c", \
             "open-youtube \"$0\"", u, NULL \
        } \
}

#define SETURI_HISTORY(p)       { .v = (char *[]){ "/bin/sh", "-c", \
"prop=\"$(tac ~/.surf/history.txt | dmenu -l 10 -i | cut -d ' ' -f 3)\" &&" \
"xprop -id $1 -f $0 8s -set $0 \"$prop\"", \
p, winid, NULL } }

#define SETURI_BOOKMARKS(p)       { .v = (char *[]){ "/bin/sh", "-c", \
"prop=\"$(get-bookmark)\" &&" \
"xprop -id $1 -f $0 8s -set $0 \"$prop\"", \
p, winid, NULL } }

/* styles */
/*
 * The iteration will stop at the first match, beginning at the beginning of
 * the list.
 */
static SiteSpecific styles[] = {
	/* regexp               file in $styledir */
	{ "https://wiki.archlinux.org/.*", "archwiki.css" },
	{ "https://app.slack.com/client/.*", "slack.css" },
	{ "https://duckduckgo.(com|co.uk)/.*", "duckduckgo.css" },
	{ "https://(.*\.)?duckduckgo.(com|co.uk)/.*", "duckduckgo.css" },
	{ "https://(.*\.)?reddit.com/.*", "reddit.css" },
	{ "file://.*", "none.css" },
	{ ".*",                 "default.css" }
};

/* certificates */
/*
 * Provide custom certificate for urls
 */
static SiteSpecific certs[] = {
	/* regexp               file in $certdir */
	{ "://suckless\\.org/", "suckless.org.crt" },
};

static SearchEngine searchengines[] = {
	{ "ddg",   "http://duckduckgo.co.uk/?q=%s"   },
	// Google
	{ "goog", "https://google.com/search?q=%s" },
	// Arch User Repository
	{ "aur", "https://aur.archlinux.org/packages/?O=0&K=%s" },
	// Arch Wiki
	{ "aw", "https://wiki.archlinux.org/?search=%s" },
	// Wordpress
	{ "wps", "https://developer.wordpress.org/?s=%s" },
	// Wordpress - Only functions
	{ "wpf", "https://developer.wordpress.org/?s=%s&post_type%5B%5D=wp-parser-function" },
	// Wordpress - Only hooks
	{ "wph", "https://developer.wordpress.org/?s=%s&post_type%5B%5D=wp-parser-hook" },
	// Wordpress - Only classes
	{ "wpc", "https://developer.wordpress.org/?s=%s&post_type%5B%5D=wp-parser-class" },
	// Wordpress - Only methods
	{ "wpm", "https://developer.wordpress.org/?s=%s&post_type%5B%5D=wp-parser-method" },
	// PHP.net
	{ "phps", "https://secure.php.net/manual-lookup.php?pattern=%s&scope=quickref" },
	// Can I Use
	{ "ciu", "https://caniuse.com/#search=%s" },
	// Mozzila Developer Network web docs
	{ "mdn", "https://developer.mozilla.org/en-US/search?q=%s" },
	// GitHub
	{ "gh", "https://github.com/search?q=%s" },
	// Youtube
	{ "yt", "https://www.youtube.com/results?search_query=%s" },
	// Wikipedia
	{ "wiki", "https://en.wikipedia.org/wiki/%s" },
};

#define MODKEY GDK_CONTROL_MASK

/* hotkeys */
/*
 * If you use anything else but MODKEY and GDK_SHIFT_MASK, don't forget to
 * edit the CLEANMASK() macro.
 */
static Key keys[] = {
	/* modifier              keyval          function    arg */
	{ 0,                     GDK_KEY_o,      spawn,      SETPROP("_SURF_URI", "_SURF_GO", PROMPT_GO) },
    { MODKEY               , GDK_KEY_o,      spawn,      SETURI_HISTORY("_SURF_GO") },
    { MODKEY               , GDK_KEY_b,      spawn,      SETURI_BOOKMARKS("_SURF_GO") },
	{ 0,                     GDK_KEY_slash,  spawn,      SETPROP("_SURF_FIND", "_SURF_FIND", PROMPT_FIND) },

	{ 0,                     GDK_KEY_i,      insert,     { .i = 1 } },
	{ 0,                     GDK_KEY_Escape, insert,     { .i = 0 } },

	{ 0,                     GDK_KEY_c,      stop,       { 0 } },

	/* Reload with or without cache */
	{ MODKEY,                GDK_KEY_r,      reload,     { .i = 1 } },
	{ 0,                     GDK_KEY_r,      reload,     { .i = 0 } },

	/* Forward and backwards in history */
	{ 0|GDK_SHIFT_MASK,      GDK_KEY_l,      navigate,   { .i = +1 } },
	{ 0|GDK_SHIFT_MASK,      GDK_KEY_h,      navigate,   { .i = -1 } },

	/* vertical and horizontal scrolling, in viewport percentage */
	{ 0,                     GDK_KEY_j,      scrollv,    { .i = +10 } },
	{ 0,                     GDK_KEY_k,      scrollv,    { .i = -10 } },
	{ 0,                     GDK_KEY_l,      scrollh,    { .i = +10 } },
	{ 0,                     GDK_KEY_h,      scrollh,    { .i = -10 } },


	/* Zooming */
	{ 0,                     GDK_KEY_minus,  zoom,       { .i = -1 } },
	{ 0|GDK_SHIFT_MASK,      GDK_KEY_plus,   zoom,       { .i = +1 } },
	{ 0,                     GDK_KEY_equal,  zoom,       { .i = 0  } },

	{ 0,                     GDK_KEY_p,      clipboard,  { .i = 1 } },
	{ 0,                     GDK_KEY_y,      clipboard,  { .i = 0 } },
	/* Youtube */
	{ 0|GDK_SHIFT_MASK,      GDK_KEY_y,      play_external,  { 0 } },

	/* next / previous search match */
	{ 0,                     GDK_KEY_n,      find,       { .i = +1 } },
	{ 0|GDK_SHIFT_MASK,      GDK_KEY_n,      find,       { .i = -1 } },

	{ MODKEY,                GDK_KEY_p,      print,      { 0 } },
	{ MODKEY,                GDK_KEY_t,      showcert,   { 0 } },

	{ MODKEY|GDK_SHIFT_MASK, GDK_KEY_a,      togglecookiepolicy, { 0 } },
	{ 0,                     GDK_KEY_F11,    togglefullscreen, { 0 } },
	{ MODKEY|GDK_SHIFT_MASK, GDK_KEY_o,      toggleinspector, { 0 } },

	{ MODKEY|GDK_SHIFT_MASK, GDK_KEY_c,      toggle,     { .i = CaretBrowsing } },
	{ MODKEY|GDK_SHIFT_MASK, GDK_KEY_f,      toggle,     { .i = FrameFlattening } },
	{ MODKEY|GDK_SHIFT_MASK, GDK_KEY_g,      toggle,     { .i = Geolocation } },
	{ MODKEY|GDK_SHIFT_MASK, GDK_KEY_s,      toggle,     { .i = JavaScript } },
	{ MODKEY|GDK_SHIFT_MASK, GDK_KEY_i,      toggle,     { .i = LoadImages } },
	{ MODKEY|GDK_SHIFT_MASK, GDK_KEY_v,      toggle,     { .i = Plugins } },
	{ MODKEY|GDK_SHIFT_MASK, GDK_KEY_b,      toggle,     { .i = ScrollBars } },
	{ MODKEY|GDK_SHIFT_MASK, GDK_KEY_t,      toggle,     { .i = StrictTLS } },
	{ MODKEY|GDK_SHIFT_MASK, GDK_KEY_m,      toggle,     { .i = Style } },
};

/* button definitions */
/* target can be OnDoc, OnLink, OnImg, OnMedia, OnEdit, OnBar, OnSel, OnAny */
static Button buttons[] = {
	/* target       event mask      button  function        argument        stop event */
	{ OnLink,       0,              2,      clicknewwindow, { .i = 0 },     1 },
	{ OnLink,       MODKEY,         2,      clicknewwindow, { .i = 1 },     1 },
	{ OnLink,       MODKEY,         1,      clicknewwindow, { .i = 1 },     1 },
	{ OnAny,        0,              8,      clicknavigate,  { .i = -1 },    1 },
	{ OnAny,        0,              9,      clicknavigate,  { .i = +1 },    1 },
	{ OnMedia,      MODKEY,         1,      clickexternplayer, { 0 },       1 },
};
