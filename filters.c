Uint32 make_pixel(Uint8 r, Uint8 g, Uint8 b)
{
	return (0xFFU << 24) | ((Uint32)r << 16) | ((Uint32)g << 8) | (Uint32)b;
}

void unpack_pixel_rgb(Uint32 pixel, Uint8 *r, Uint8 *g, Uint8 *b)
{
	*r = pixel >> 16 & 0xFF;
	*g = pixel >> 8 & 0xFF;
	*b = pixel & 0xFF;
}

Uint8 clamp_byte(int x)
{
	return (x > 0xFF) ? 0xFF : x;
}

SDL_Texture * grayscale(SDL_Surface *surface, SDL_Renderer *renderer)
{
	SDL_Surface *s = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_ARGB8888);

	Uint32 * pixels = (Uint32 *)s->pixels;
	for (int y = 0; y < s->h; y++) {
		for (int x = 0; x < s->w; x++) {
			Uint32 *pixel = pixels + y*s->w + x;
			Uint8 r, g, b;
			unpack_pixel_rgb(*pixel, &r, &g, &b);
			// Apparently using a weighted average gives better
			// results than taking the mean since human vision is
			// most sensitive to green and least sensitive to blue.
			Uint8 v = 0.212671f * r + 0.715160f * g + 0.072169f * b;
			*pixel = make_pixel(v, v, v);
		}
	}

	SDL_Texture *t = SDL_CreateTextureFromSurface(renderer, s);
	SDL_DestroySurface(s);
	return t;
}

SDL_Texture * sepia(SDL_Surface *surface, SDL_Renderer *renderer)
{
	SDL_Surface *s = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_ARGB8888);

	Uint32 * pixels = (Uint32 *)s->pixels;
	for (int y = 0; y < s->h; y++) {
		for (int x = 0; x < s->w; x++) {
			Uint32 *pixel = pixels + y*s->w + x;
			Uint8 r, g, b;
			unpack_pixel_rgb(*pixel, &r, &g, &b);
			int sepia_r = clamp_byte(0.393 * r + 0.769 * g + 0.189 * b);
			int sepia_g = clamp_byte(0.349 * r + 0.686 * g + 0.168 * b);
			int sepia_b = clamp_byte(0.272 * r + 0.534 * g + 0.131 * b);
			*pixel = make_pixel(sepia_r, sepia_g, sepia_b);
		}
	}

	SDL_Texture *t = SDL_CreateTextureFromSurface(renderer, s);
	SDL_DestroySurface(s);
	return t;
}

SDL_Texture * reflect(SDL_Surface *surface, SDL_Renderer *renderer)
{
	SDL_Surface *s = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_ARGB8888);

	Uint32 * pixels = (Uint32 *)s->pixels;
	for (int y = 0; y < s->h; y++) {
		for (int x = 0; x < s->w/2; x++) {
			Uint32 *left_pixel = pixels + y*s->w + x;
			Uint32 *right_pixel = pixels + y*s->w + (s->w - x - 1);
			Uint32 tmp_pixel = *left_pixel;
			*left_pixel = *right_pixel;
			*right_pixel = tmp_pixel;
		}
	}

	SDL_Texture *t = SDL_CreateTextureFromSurface(renderer, s);
	SDL_DestroySurface(s);
	return t;
}

SDL_Texture * blur(SDL_Surface *surface, SDL_Renderer *renderer)
{
	SDL_Surface *s = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_ARGB8888);
	SDL_Surface *new_surface = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_ARGB8888);

	Uint32 *pixels = (Uint32 *)s->pixels;
	Uint32 *new_pixels = (Uint32 *)new_surface->pixels;
	for (int y = 0; y < s->h; y++) {
		for (int x = 0; x < s->w; x++) {
			int n = 0;
			int total_r = 0;
			int total_g = 0;
			int total_b = 0;

			for (int y_offset = -1; y_offset < 2; y_offset++) {
				for (int x_offset = -1; x_offset < 2; x_offset++) {
					if (y+y_offset < 0 || y+y_offset >= s->h ||
					    x+x_offset < 0 || x+x_offset >= s->w) {
						continue;
					}

					Uint32 pixel = pixels[(y+y_offset)*s->w + x+x_offset];
					Uint8 r, g, b;
					unpack_pixel_rgb(pixel, &r, &g, &b);
					total_r += r;
					total_g += g;
					total_b += b;
					n++;
				}
			}
			new_pixels[y*s->w + x] = make_pixel(total_r/n, total_g/n, total_b/n);
		}
	}

	SDL_Texture *t = SDL_CreateTextureFromSurface(renderer, new_surface);
	SDL_DestroySurface(s);
	SDL_DestroySurface(new_surface);
	return t;
}

SDL_Texture * edges(SDL_Surface *surface, SDL_Renderer *renderer)
{
	SDL_Surface *s = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_ARGB8888);
	SDL_Surface *new_surface = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_ARGB8888);

	static const int sobel_x[9] = { -1, 0, 1, -2, 0, 2, -1, 0, 1 };
	static const int sobel_y[9] = { -1, -2, -1, 0, 0, 0, 1, 2, 1 };

	Uint32 * pixels = (Uint32 *)s->pixels;
	Uint32 * new_pixels = (Uint32 *)new_surface->pixels;
	for (int y = 0; y < s->h; y++) {
		for (int x = 0; x < s->w; x++) {
			int rx, gx, bx, ry, gy, by;
			rx = gx = bx = ry = gy = by = 0;
			for (int y_offset = -1; y_offset < 2; y_offset++) {
				for (int x_offset = -1; x_offset < 2; x_offset++) {
					if (y+y_offset < 0 || y+y_offset >= s->h ||
					    x+x_offset < 0 || x+x_offset >= s->w) {
						continue;
					}

					Uint32 pixel = pixels[(y+y_offset)*s->w + x+x_offset];
					int sobel_index = (y_offset+1)*3 + (x_offset+1);
					Uint8 r, g, b;
					unpack_pixel_rgb(pixel, &r, &g, &b);
					rx += r * sobel_x[sobel_index];
					gx += g * sobel_x[sobel_index];
					bx += b * sobel_x[sobel_index];
					ry += r * sobel_y[sobel_index];
					gy += g * sobel_y[sobel_index];
					by += b * sobel_y[sobel_index];
				}
			}

			Uint8 r = clamp_byte(SDL_sqrt(rx*rx + ry*ry));
			Uint8 g = clamp_byte(SDL_sqrt(gx*gx + gy*gy));
			Uint8 b = clamp_byte(SDL_sqrt(bx*bx + by*by));
			new_pixels[y*s->w + x] = make_pixel(r, g, b);
		}
	}

	SDL_Texture *t = SDL_CreateTextureFromSurface(renderer, new_surface);
	SDL_DestroySurface(s);
	SDL_DestroySurface(new_surface);
	return t;
}
