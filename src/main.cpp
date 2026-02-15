#include <iostream>
#include <ctime>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
#include <fstream>
#include <vector>

#include <string>

#include <cctype>
#include <iomanip>

#include <cairo/cairo.h>
#include <cairo/cairo-pdf.h>

struct Glyph
{
    std::string text;
    int x, y;
};

std::vector<Glyph> lastPage;

std::string generateFileName()
{
    // get current date to be used as the name of the output pdf file
    // this because each page/sheet has to be unique

    // 1. Get current time
    std::time_t now = std::time(nullptr);

    // 2. Convert to local time structure
    // Note: Use localtime_s (MSVC) or localtime_r (POSIX) for thread safety if needed
    std::tm *localTime = std::localtime(&now);

    // 3. Format to YYYYMMDDHHMMSS
    std::ostringstream oss;
    oss << std::put_time(localTime, "%Y%m%d%H%M%S");

    // Convert to char pointer
    return "output/" + oss.str();
    // -------------------------------
}

std::string filename = generateFileName();

void exportToPDF(
    const std::vector<Glyph> &page)
{
    const int width = 800;
    const int height = 1000;

    cairo_surface_t *surface =
        cairo_pdf_surface_create(filename.c_str(), width, height);

    cairo_t *cr = cairo_create(surface);

    // Background (paper)
    cairo_set_source_rgb(cr, 0.96, 0.96, 0.94);
    cairo_paint(cr);

    // Ink
    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_select_font_face(
        cr,
        "Courier",
        CAIRO_FONT_SLANT_NORMAL,
        CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, 24);

    for (const auto &g : page)
    {
        cairo_move_to(cr, g.x, g.y + 24); // baseline correction
        cairo_show_text(cr, g.text.c_str());
    }

    cairo_show_page(cr);

    cairo_destroy(cr);
    cairo_surface_destroy(surface);
}

int main()
{
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    TTF_Init();
    Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048);

    SDL_Window *window = SDL_CreateWindow(
        "Typewriter",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        800, 1000,
        SDL_WINDOW_SHOWN);

    SDL_Renderer *renderer = SDL_CreateRenderer(
        window, -1, SDL_RENDERER_ACCELERATED);

    TTF_Font *font = TTF_OpenFont("resources/fonts/courier.ttf", 24);

    // Load sounds
    Mix_Chunk *hammer_hit = Mix_LoadWAV("resources/sounds/typewriter-keydown.wav");
    Mix_Chunk *spacebar = Mix_LoadWAV("resources/sounds/typewriter-spacebar.wav");
    Mix_Chunk *carriage_return = Mix_LoadWAV("resources/sounds/typewriter-carriage-return-main.wav");

    std::vector<Glyph> page;
    std::ofstream journal("output/session.log", std::ios::app);

    int x = 50;
    int y = 50;
    const int lineHeight = 30;

    bool running = true;
    SDL_StartTextInput();

    while (running)
    {
        SDL_Event e;
        while (SDL_PollEvent(&e))
        {
            if (e.type == SDL_QUIT)
                running = false;

            // writing events
            if (e.type == SDL_TEXTINPUT)
            {
                std::string text = " "; // utf8 character
                
                // If spacebar
                if (e.key.keysym.sym == SDLK_SPACE)
                {
                    page.push_back({text, x, y});
                    journal << text << " " << x << " " << y << "\n";
                    journal.flush();

                    int w, h;
                    TTF_SizeUTF8(font, text.c_str(), &w, &h);
                    x += w;
                    // x += 14;

                    Mix_PlayChannel(-1, spacebar, 0);
                }
                // Normal text wiriting (letters)
                else
                {
                    text = e.text.text;

                    // only if real text input (no blank-empty, like from a tabulator)
                    if (!text.empty())
                    {
                        page.push_back({text, x, y});
                        journal << text << " " << x << " " << y << "\n";
                        journal.flush();

                        Mix_PlayChannel(-1, hammer_hit, 0);

                        x += 14;
                        if (x > 750)
                        {
                            // Auto carriage return
                            x = 50;
                            y += lineHeight;
                            Mix_PlayChannel(-1, carriage_return, 0);
                        }
                    }
                }
            }

            // Cursor movement events
            if (e.type == SDL_KEYDOWN)
            {
                switch (e.key.keysym.sym)
                {
                case SDLK_UP:
                    y -= lineHeight;
                    break;
                case SDLK_DOWN:
                    y += lineHeight;
                    break;
                case SDLK_LEFT:
                    x -= 14;
                    break;
                case SDLK_RIGHT:
                    x += 14;
                    break;

                // Manual carriage return (new line)
                case SDLK_RETURN:
                    x = 50;
                    y += lineHeight;
                    Mix_PlayChannel(-1, carriage_return, 0);
                    break;

                default:
                    break;
                }
            }
        }

        // This is just for the cursor actually
        SDL_SetRenderDrawColor(renderer, 245, 245, 240, 255);
        SDL_RenderClear(renderer);

        SDL_Color ink = {0, 0, 0, 200};

        SDL_Surface *surf = TTF_RenderText_Blended(font, "|", ink);
        SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);

        SDL_Rect dst = {x, y, surf->w, surf->h};
        SDL_RenderCopy(renderer, tex, NULL, &dst);

        SDL_FreeSurface(surf);
        SDL_DestroyTexture(tex);
        // ------------------------

        // if something has been added (update)
        if (lastPage.size() < page.size())
        {
            // and only then, reflect update to pdf
            exportToPDF(page);
            lastPage = page;
            // this can be improved for performance if we check for updates each certain time,
            // not all the time (for cpu resting: save lot of updates to pdf instead one by one)
        }

        // this render the glyphs on screen
        for (const auto &g : page)
        {
            SDL_Color ink = {0, 0, 0, 200};

            SDL_Surface *surf = TTF_RenderUTF8_Blended(font, g.text.c_str(), ink);
            SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, surf);

            SDL_Rect dst = {g.x, g.y, surf->w, surf->h};
            SDL_RenderCopy(renderer, tex, NULL, &dst);

            SDL_FreeSurface(surf);
            SDL_DestroyTexture(tex);
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    SDL_StopTextInput();
    journal.close();

    Mix_FreeChunk(hammer_hit);
    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    Mix_CloseAudio();
    TTF_Quit();
    SDL_Quit();

    return 0;
}
