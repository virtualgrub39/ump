#include <locale.h>
#include <miniaudio.h>
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <taglib/tag_c.h>
#include <glib.h>

static ma_engine engine;

struct file_info {
    const char* file_path;
    const char* title;
    const char* artist;
    const char* album;
    const char* genre;
    unsigned int year;
    int bitrate_kbps;
    int length_secs;
    int channels;
    int samplerate_hz;
};

bool
get_file_info(const char* path, struct file_info* info)
{
    TagLib_File* file = taglib_file_new(path);
    if (!file) {
        perror("taglib_file_new");
        return false;
    }
    TagLib_Tag* tag = taglib_file_tag(file);
    if (!tag) {
        fprintf(stderr, "No tags in %s\n", path);
        taglib_file_free(file);
        return false;
    }
    const TagLib_AudioProperties* ap = taglib_file_audioproperties(file);
    if (!ap) {
        fprintf(stderr, "Couldn't read audio properties of %s\n", path);
        taglib_file_free(file);
        return false;
    }

    info->title = taglib_tag_title(tag);
    info->artist = taglib_tag_artist(tag);
    info->album = taglib_tag_album(tag);
    info->year = taglib_tag_year(tag);
    info->genre = taglib_tag_genre(tag);
    info->bitrate_kbps = taglib_audioproperties_bitrate(ap);
    info->length_secs = taglib_audioproperties_length(ap);
    info->channels = taglib_audioproperties_channels(ap);
    info->samplerate_hz = taglib_audioproperties_samplerate(ap);

    taglib_file_free(file);
    return true;
}

typedef struct {
    struct file_info info;
    ma_sound sound;
} Song;

bool
song_load(const char* path, Song* song)
{
    if (!song)
        return false;
    if (!get_file_info(path, &song->info))
        return false;

    if (ma_sound_init_from_file(&engine, path, 0, NULL, NULL, &song->sound) !=
        MA_SUCCESS)
        return false;

    return true;
}

void
song_resume(Song* song)
{
    ma_sound_start(&song->sound);
}

void
song_pause(Song* song)
{
    ma_sound_stop_with_fade_in_milliseconds(&song->sound, 1);
}

void
song_unload(gpointer data, gpointer user_data)
{
    Song* song = (Song*)data;
    (void)user_data;
    ma_sound_uninit(&song->sound);
}

typedef struct {
    GPtrArray* LoadedMusic;
    guint current_song_idx;
} Player;

Player
player_new()
{
    return (Player) {
        .LoadedMusic = g_ptr_array_new(),
        .current_song_idx = 0,
    };
}

void
player_destroy(Player* p)
{
    g_ptr_array_foreach(p->LoadedMusic, song_unload, NULL);
    g_ptr_array_free(p->LoadedMusic, TRUE);
}

bool
player_load_from_file(Player* p, const char* path)
{
    Song* song = g_malloc(sizeof(Song));
    if (!song_load(path, song)) {
        g_free(song);
        return false;
    }

    g_ptr_array_add(p->LoadedMusic, song);

    return true;
}

void
player_set_current_song_idx(Player* p, guint idx)
{
    p->current_song_idx = idx;
}

void
player_resume(Player* p)
{
    g_assert_false(p->current_song_idx >= p->LoadedMusic->len);
    Song* current_song = g_ptr_array_index(p->LoadedMusic, p->current_song_idx);
    song_resume(current_song);
}

void
player_pause(Player* p)
{
    g_assert_false(p->current_song_idx >= p->LoadedMusic->len);
    Song* current_song = g_ptr_array_index(p->LoadedMusic, p->current_song_idx);
    song_pause(current_song);
}

struct file_info*
player_get_current_info(const Player* p)
{
    g_assert_false(p->current_song_idx >= p->LoadedMusic->len);
    Song* current_song = g_ptr_array_index(p->LoadedMusic, p->current_song_idx);
    return &current_song->info;
}

int
main(int argc, char* argv[])
{
    if (!setlocale(LC_ALL, "C.utf8")) {
        clear();
        mvprintw(1, 1, "Failed to set UTF-8 locale\n");
        goto ncurses_deinit;
    }

    initscr();
    cbreak();
    noecho();

    if (argc < 2) {
        clear();
        mvprintw(1, 1, "Usage: %s <audio-file>\n", argv[0]);
        goto ncurses_deinit;
    }

    if (ma_engine_init(NULL, &engine) != MA_SUCCESS) {
        clear();
        mvprintw(1, 1, "Failed to init audio engine\n");
        goto ncurses_deinit;
    }

    Player player = player_new();
    if (!player_load_from_file(&player, argv[1])) {
        clear();
        mvprintw(1, 1, "Failed to load audio file\n");
        goto miniaudio_deinit;
    }

    struct file_info* info = player_get_current_info(&player);

    mvprintw(1, 1, "Title     : %s", info->title);
    mvprintw(2, 1, "Artist    : %s", info->artist);
    mvprintw(3, 1, "Album     : %s", info->album);
    mvprintw(4, 1, "Year      : %u", info->year);
    mvprintw(5, 1, "Genre     : %s", info->genre);
    mvprintw(6, 1, "Bitrate   : %u [kb/s]", info->bitrate_kbps);
    mvprintw(7, 1, "Length    : %u [s]", info->length_secs);
    mvprintw(8, 1, "Channels  : %u", info->channels);
    mvprintw(9, 1, "Samplerate: %u [hz]", info->samplerate_hz);
    refresh();

    taglib_tag_free_strings();

    player_resume(&player);
    
    getch();

// ump_deinit:
    player_destroy(&player);
miniaudio_deinit:
    ma_engine_uninit(&engine);
ncurses_deinit:
    getch();
    endwin();
    return 0;
}
