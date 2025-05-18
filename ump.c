#include <locale.h>
#include <miniaudio.h>
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <taglib/tag_c.h>

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

typedef struct sound {
    struct file_info info;
    ma_sound sound;
} Sound;

bool
sound_load(const char* path, Sound* sound)
{
    if (!sound)
        return false;
    if (!get_file_info(path, &sound->info))
        return false;

    if (ma_sound_init_from_file(&engine, path, 0, NULL, NULL, &sound->sound) !=
        MA_SUCCESS)
        return false;

    return true;
}

void
sound_resume(Sound* sound)
{
    ma_sound_start(&sound->sound);
}

void
sound_pause(Sound* sound)
{
    ma_sound_stop_with_fade_in_milliseconds(&sound->sound, 1);
}

void
sound_unload(Sound* sound)
{
    ma_sound_uninit(&sound->sound);
}

int
main(int argc, char* argv[])
{
    if (!setlocale(LC_ALL, "C.utf8")) {
        fprintf(stderr, "Failed to set UTF-8 locale\n");
        return 1;
    }

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <audio-file>\n", argv[0]);
        return 1;
    }

    if (ma_engine_init(NULL, &engine) != MA_SUCCESS) {
        fprintf(stderr, "Failed to init audio engine\n");
        return 1;
    }

    Sound song = { 0 };
    if (!sound_load(argv[1], &song)) {
        fprintf(stderr, "Failed to load audio file\n");
        return 1;
    }

    initscr();
    cbreak();
    noecho();

    mvprintw(1, 1, "Title     : %s", song.info.title);
    mvprintw(2, 1, "Artist    : %s", song.info.artist);
    mvprintw(3, 1, "Album     : %s", song.info.album);
    mvprintw(4, 1, "Year      : %u", song.info.year);
    mvprintw(5, 1, "Genre     : %s", song.info.genre);
    mvprintw(6, 1, "Bitrate   : %u [kb/s]", song.info.bitrate_kbps);
    mvprintw(7, 1, "Length    : %u [s]", song.info.length_secs);
    mvprintw(8, 1, "Channels  : %u", song.info.channels);
    mvprintw(9, 1, "Samplerate: %u [hz]", song.info.samplerate_hz);
    refresh();

    taglib_tag_free_strings();

    sound_resume(&song);
    getch();
    sound_unload(&song);

    endwin();
    ma_engine_uninit(&engine);

    return 0;
}
