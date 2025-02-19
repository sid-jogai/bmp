#define SDL_MAIN_USE_CALLBACKS 1

#include "SDL3/SDL.h"
#include "SDL3/SDL_main.h"

#include "filters.c"

#define WINDOW_WIDTH  640
#define WINDOW_HEIGHT 480

enum filter_type {
	FILTER_NONE,
	FILTER_GRAYSCALE,
	FILTER_SEPIA,
	FILTER_REFLECT,
	FILTER_BLUR,
	FILTER_EDGES,
	FILTER_MAX,
};

typedef struct {
	SDL_SystemTheme theme;
	SDL_Window *window;
	SDL_Renderer *renderer;

	enum filter_type cur_filter;
	SDL_Surface *original_surface;
	SDL_Texture *textures[FILTER_MAX];

} AppState;

static const char *const window_titles[] = {
	"bmp", "bmp - grayscale", "bmp - sepia", "bmp - reflect", "bmp - blur", "bmp - edges"
};

void load_bmp(AppState *s, const char *path)
{
	s->original_surface = SDL_LoadBMP(path);
	if (!s->original_surface) {
		SDL_Log("Error loading image: %s", SDL_GetError());
		return;
	}

	/* Since there are only five filters, just create all of the textures upfront. */
	s->textures[FILTER_NONE] = SDL_CreateTextureFromSurface(s->renderer, s->original_surface);
	s->textures[FILTER_GRAYSCALE] = grayscale(s->original_surface, s->renderer);
	s->textures[FILTER_SEPIA] = sepia(s->original_surface, s->renderer);
	s->textures[FILTER_REFLECT] = reflect(s->original_surface, s->renderer);
	s->textures[FILTER_BLUR] = blur(s->original_surface, s->renderer);
	s->textures[FILTER_EDGES] = edges(s->original_surface, s->renderer);

	SDL_SetWindowSize(s->window, s->textures[FILTER_NONE]->w, s->textures[FILTER_NONE]->h);
}

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
	if (!SDL_Init(SDL_INIT_VIDEO)) {
		SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	AppState *s = (AppState *)SDL_calloc(1, sizeof(AppState));
	if (!s) {
		return SDL_APP_FAILURE;
	}
	s->theme = SDL_GetSystemTheme();

	if (!SDL_CreateWindowAndRenderer("bmp",
					 WINDOW_WIDTH,
					 WINDOW_HEIGHT,
					 SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY,
					 &s->window,
					 &s->renderer)) {
		SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	if (argc == 2) {
		load_bmp(s, argv[1]);
	}

	*appstate = s;
	return SDL_APP_CONTINUE;
}

void select_file_callback(void *appstate, const char * const *filelist, int filter)
{
	(void)filter;
	AppState *s = (AppState *)appstate;

	if (!filelist) {
		SDL_Log("Error selecting file: %s", SDL_GetError());
		return;
	}

	if (filelist[0]) {
		load_bmp(s, filelist[0]);
	}
}


SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
	AppState *s = (AppState *)appstate;

	if (event->type == SDL_EVENT_QUIT) {
		return SDL_APP_SUCCESS;
	}

	if (event->type == SDL_EVENT_SYSTEM_THEME_CHANGED) {
		s->theme = SDL_GetSystemTheme();
	}

	if (event->type == SDL_EVENT_KEY_DOWN) {
		switch (event->key.scancode) {
		case SDL_SCANCODE_Q:
			return SDL_APP_SUCCESS;
			break;
		case SDL_SCANCODE_F:;
			SDL_ShowOpenFileDialog(select_file_callback,
					       appstate,
					       s->window,
					       NULL,
					       0,
					       NULL,
					       false);
			break;
		case SDL_SCANCODE_G:;
			s->cur_filter = FILTER_GRAYSCALE;
			break;
		case SDL_SCANCODE_S:;
			s->cur_filter = FILTER_SEPIA;
			break;
		case SDL_SCANCODE_R:;
			s->cur_filter = FILTER_REFLECT;
			break;
		case SDL_SCANCODE_B:;
			s->cur_filter = FILTER_BLUR;
			break;
		case SDL_SCANCODE_E:;
			s->cur_filter = FILTER_EDGES;
			break;
		case SDL_SCANCODE_O:;
			s->cur_filter = FILTER_NONE;
			break;
		default:
			break;
		}
	}

	SDL_SetWindowTitle(s->window, window_titles[s->cur_filter]);

	return SDL_APP_CONTINUE;
}

void draw_text(SDL_Renderer *renderer, SDL_SystemTheme theme, float x, float y, const char *s)
{
	if (theme == SDL_SYSTEM_THEME_DARK) {
		SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
	} else {
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
	}

	SDL_SetRenderScale(renderer, 2.0f, 2.0f);
	SDL_RenderDebugText(renderer, x, y, s);
	SDL_SetRenderScale(renderer, 1.0f, 1.0f);
}

void draw_bg(SDL_Renderer *renderer, SDL_SystemTheme theme)
{
	if (theme == SDL_SYSTEM_THEME_DARK) {
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
	} else {
		SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
	}

	SDL_RenderClear(renderer);
}

void show_help_text(SDL_Renderer *renderer, SDL_SystemTheme theme)
{
	draw_text(renderer, theme, 10, 10, "Press F to select a file.");
	draw_text(renderer, theme, 10, 30, "Once a file is loaded, use the ");
	draw_text(renderer, theme, 10, 40, "following keys to apply filters:");
	draw_text(renderer, theme, 20, 60, "G - Grayscale");
	draw_text(renderer, theme, 20, 80, "S - Sepia");
	draw_text(renderer, theme, 20, 100, "R - Reflect");
	draw_text(renderer, theme, 20, 120, "B - Blur");
	draw_text(renderer, theme, 20, 140, "E - Edges");
	draw_text(renderer, theme, 20, 160, "O - Original");
}

SDL_AppResult SDL_AppIterate(void *appstate)
{
	AppState *s = (AppState *)appstate;
	draw_bg(s->renderer, s->theme);

	if (s->textures[s->cur_filter]) {
		SDL_RenderTexture(s->renderer, s->textures[s->cur_filter], NULL, NULL);
	} else {
		show_help_text(s->renderer, s->theme);
	}

	SDL_RenderPresent(s->renderer);

	SDL_Delay(40);
	return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
	(void)appstate;
	(void)result;
}
