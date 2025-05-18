#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <ncurses.h>
#include <miniaudio.h>
#include <taglib/tag_c.h>

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

    TagLib_File *file = taglib_file_new(argv[1]);
    if (!file) {
        perror("taglib_file_new");
        return 1;
    }
    TagLib_Tag *tag = taglib_file_tag(file);
    if (!tag) {
        fprintf(stderr, "No tags in %s\n", argv[1]);
        taglib_file_free(file);
        return 0;
    }

    const char *title  = taglib_tag_title(tag);
    const char *artist = taglib_tag_artist(tag);
    const char *album  = taglib_tag_album(tag);
    unsigned int year  = taglib_tag_year(tag);
    const char *genre  = taglib_tag_genre(tag);

    initscr();
    cbreak();
    noecho();

    mvprintw(1, 1, "Title : %s",  title);
    mvprintw(2, 1, "Artist: %s",  artist);
    mvprintw(3, 1, "Album : %s",  album);
    mvprintw(4, 1, "Year  : %u",  year);
    mvprintw(5, 1, "Genre : %s",  genre);
    refresh();

    taglib_tag_free_strings();
    taglib_file_free(file);

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
