#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <ncurses.h>
#include <miniaudio.h>
#include <taglib/tag_c.h>

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

    info->title  = taglib_tag_title(tag);
    info->artist = taglib_tag_artist(tag);
    info->album  = taglib_tag_album(tag);
    info->year  = taglib_tag_year(tag);
    info->genre  = taglib_tag_genre(tag);
    info->bitrate_kbps = taglib_audioproperties_bitrate(ap);
    info->length_secs = taglib_audioproperties_length(ap);
    info->channels = taglib_audioproperties_channels(ap);
    info->samplerate_hz = taglib_audioproperties_samplerate(ap);

    taglib_file_free(file);
    return true;
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

    struct file_info info = { 0 };
    get_file_info(argv[1], &info);

    initscr();
    cbreak();
    noecho();

    mvprintw(1, 1, "Title     : %s",  info.title);
    mvprintw(2, 1, "Artist    : %s",  info.artist);
    mvprintw(3, 1, "Album     : %s",  info.album);
    mvprintw(4, 1, "Year      : %u",  info.year);
    mvprintw(5, 1, "Genre     : %s",  info.genre);
    mvprintw(6, 1, "Bitrate   : %u [kb/s]",  info.bitrate_kbps);
    mvprintw(7, 1, "Length    : %u [s]",  info.length_secs);
    mvprintw(8, 1, "Channels  : %u",  info.channels);
    mvprintw(9, 1, "Samplerate: %u [hz]",  info.samplerate_hz);
    refresh();

    taglib_tag_free_strings();

    ma_engine engine;
    if (ma_engine_init(NULL, &engine) != MA_SUCCESS) {
        endwin();
        fprintf(stderr, "Failed to init audio engine\n");
        return 1;
    }
    ma_engine_play_sound(&engine, argv[1], NULL);

    getch();
    endwin();
    ma_engine_uninit(&engine);

    return 0;
}
