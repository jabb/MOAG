
#include "client.h"

#define X true
#define _ false

#define TANK_WIDTH  18
#define TANK_HEIGHT 14
const bool tanksprite[TANK_WIDTH * TANK_HEIGHT] =
{
    _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
    _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
    _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
    _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
    _,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,_,
    _,_,_,_,_,_,_,_,X,X,_,_,_,_,_,_,_,_,
    _,_,_,_,_,_,_,X,X,X,X,_,_,_,_,_,_,_,
    _,_,_,_,_,_,X,X,X,X,X,X,_,_,_,_,_,_,
    _,_,_,X,X,X,X,X,X,X,X,X,X,X,X,_,_,_,
    _,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,
    X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,X,
    X,X,X,_,X,X,_,X,X,_,X,X,_,X,X,_,X,X,
    _,X,_,_,X,X,_,X,_,_,X,_,_,X,_,_,X,_,
    _,_,X,X,X,X,X,X,X,X,X,X,X,X,X,X,_,_,
};

#define CRATE_WIDTH  9
#define CRATE_HEIGHT 9
bool cratesprite[CRATE_WIDTH * CRATE_HEIGHT] =
{
    _,X,X,X,X,X,X,X,_,
    X,X,_,_,_,_,_,X,X,
    X,_,X,_,_,_,X,_,X,
    X,_,_,X,_,X,_,_,X,
    X,_,_,_,X,_,_,_,X,
    X,_,_,X,_,X,_,_,X,
    X,_,X,_,_,_,X,_,X,
    X,X,_,_,_,_,_,X,X,
    _,X,X,X,X,X,X,X,_,
};

#define BULLET_WIDTH  3
#define BULLET_HEIGHT 3
bool bulletsprite[BULLET_WIDTH * BULLET_HEIGHT] =
{
    _,X,_,
    X,X,X,
    _,X,_,
};

struct chatline chatlines[CHAT_LINES] = {{0}};

char *typing_str = NULL;
bool kleft = false;
bool kright = false;
bool kup = false;
bool kdown = false;
bool kfire = false;

void draw_tank(int x, int y, int turretangle, bool facingleft)
{
    draw_sprite(x, y, COLOR(240, 240, 240), tanksprite, TANK_WIDTH, TANK_HEIGHT);

    /* 9 is the length of the cannon. */
    int ex = 9 * cos(DEG2RAD(turretangle)) * (facingleft ? -1 : 1);
    int ey = 9 * sin(DEG2RAD(turretangle)) * -1;
    draw_line(x + TANK_WIDTH / 2, y + TANK_HEIGHT / 2,
              x + TANK_WIDTH / 2 + ex, y + TANK_HEIGHT / 2 + ey,
              COLOR(240, 240, 240));
}

void draw_crate(int x, int y)
{
    draw_sprite(x, y, COLOR_BROWN, cratesprite, CRATE_WIDTH, CRATE_HEIGHT);
}

void draw_bullets(struct moag *m)
{
    for (int i = 0; i < MAX_BULLETS; i++)
    {
        struct bullet *b = &m->bullets[i];
        if (b->active)
            draw_sprite(b->x, b->y, COLOR_RED, bulletsprite, BULLET_WIDTH, BULLET_HEIGHT);
    }
}

void del_chat_line(void)
{
    if(chatlines[0].str && chatlines[0].expire<SDL_GetTicks()){
        free(chatlines[0].str);
        for(int i=0;i<CHAT_LINES-1;i++){
            chatlines[i].expire=chatlines[i+1].expire;
            chatlines[i].str=chatlines[i+1].str;
        }
        chatlines[CHAT_LINES-1].str=NULL;
    }
}

void add_chat_line(char* str)
{
    int i=0;
    while(chatlines[i].str)
        if(++i>=CHAT_LINES){
            chatlines[0].expire=0;
            del_chat_line();
            i=CHAT_LINES-1;
            break;
        }
    chatlines[i].str=str;
    chatlines[i].expire=SDL_GetTicks()+CHAT_EXPIRETIME;
}

void draw(struct moag *m)
{
    for (int x = 0; x < LAND_WIDTH; ++x) {
        for (int y = 0; y < LAND_HEIGHT; ++y) {
            if (get_land_at(m, x, y))
                set_pixel(x, y, COLOR(155, 155, 155));
        }
    }
    for(int i=0;i<MAX_PLAYERS;i++)
        if(m->tanks[i].active) {
            draw_tank(m->tanks[i].x - 9,
                      m->tanks[i].y - 13,
                      m->tanks[i].angle,
                      m->tanks[i].facingleft);
            draw_string_centered(m->tanks[i].x,
                                 m->tanks[i].y - 36,
                                 COLOR(240, 240, 240),
                                 m->tanks[i].name);
        }
    if(m->crate.active)
        draw_crate(m->crate.x-4,m->crate.y-8);
    draw_bullets(m);
    del_chat_line();
    for(int i=0;i<CHAT_LINES;i++)
        if(chatlines[i].str)
        {
            draw_string(4,4+12*i, COLOR_WHITE, chatlines[i].str);
            int w, h;
            get_string_size(chatlines[i].str, &w, &h);
        }
    if (typing_str){
        draw_block(6,8+12*(CHAT_LINES),4,4,COLOR(210,210,210));
        draw_string(16,4+12*(CHAT_LINES),COLOR(210,210,210),typing_str);
    }
}

void on_receive(struct moag *m, ENetEvent *ev)
{
    size_t pos = 0;
    unsigned char *packet = ev->packet->data;

    char chunk_type = read8(packet, &pos);

    switch(chunk_type) {
    case LAND_CHUNK:
        read_land_chunk(m, packet, ev->packet->dataLength);
        break;

    case TANK_CHUNK: {
        int type = read8(packet, &pos);
        int id = read8(packet, &pos);

        assert(id >= 0 && id <= MAX_PLAYERS);

        if (type == SPAWN) {
            m->tanks[id].active = true;

            m->tanks[id].x = read16(packet, &pos);
            m->tanks[id].y = read16(packet, &pos);
            char angle = read8(packet, &pos);

            m->tanks[id].facingleft = false;
            if (angle < 0){
                angle = -angle;
                m->tanks[id].facingleft = true;
            }
            m->tanks[id].angle = angle;
        }
        else if (type == MOVE) {
            m->tanks[id].x = read16(packet, &pos);
            m->tanks[id].y = read16(packet, &pos);
            char angle = read8(packet, &pos);

            m->tanks[id].facingleft = false;
            if (angle < 0){
                angle = -angle;
                m->tanks[id].facingleft = true;
            }
            m->tanks[id].angle = angle;
        }
        else if (type == KILL) {
            m->tanks[id].x = -1;
            m->tanks[id].y = -1;
            m->tanks[id].active = false;
        }
        else {
            DIE("Invalid TANK_CHUNK type.\n");
        }
        break;
    }
    case BULLET_CHUNK: {
        int type = read8(packet, &pos);
        int id = read8(packet, &pos);

        if (type == SPAWN) {
            m->bullets[id].active = true;
            m->bullets[id].x = read16(packet, &pos);
            m->bullets[id].y = read16(packet, &pos);
        }
        else if (type == MOVE) {
            m->bullets[id].x = read16(packet, &pos);
            m->bullets[id].y = read16(packet, &pos);
        }
        else if (type == KILL) {
            m->bullets[id].active = false;
        }
        else {
            DIE("Invalid BULLET_CHUNK type.\n");
        }
        break;
    }
    case SERVER_MSG_CHUNK: {
        int id = read8(packet, &pos);
        char cmd = read8(packet, &pos);
        unsigned char len = read8(packet, &pos);
        switch(cmd){
        case CHAT: {
            int namelen=strlen(m->tanks[id].name);
            int linelen=namelen+len+4;
            char* line=malloc(linelen);
            line[0]='<';
            for(int i=0;i<namelen;i++)
                line[i+1]=m->tanks[id].name[i];
            line[namelen+1]='>';
            line[namelen+2]=' ';
            for (int i = 0; i < len; ++i)
                line[namelen + 3 + i] = read8(packet, &pos);
            line[linelen-1]='\0';
            add_chat_line(line);
            break;
        }
        case NAME_CHANGE: {
            if(len<1 || len>15){ // error!
                break;
            }
            for (int i = 0; i < len; ++i)
                m->tanks[id].name[i] = read8(packet, &pos);
            m->tanks[id].name[len]='\0';
            break;
        }
        case SERVER_NOTICE: { //server notice
            char* line=malloc(len+1);
            for (int i = 0; i < len; ++i)
                line[i] = read8(packet, &pos);
            line[len]='\0';
            add_chat_line(line);
            break;
        }
        default:
            break;
        }
        break;
    }
    case CRATE_CHUNK: {
        int type = read8(packet, &pos);

        if (type == SPAWN) {
            m->crate.active = true;
            m->crate.x = read16(packet, &pos);
            m->crate.y = read16(packet, &pos);
        }
        else if (type == MOVE) {
            m->crate.x = read16(packet, &pos);
            m->crate.y = read16(packet, &pos);
        }
        else if (type == KILL) {
            m->crate.active = false;
        }
        else {
            DIE("Invalid CRATE_CHUNK type.\n");
        }
        break;
    }
    default:
        DIE("Invalid CHUNK type.\n");
        break;
    }
}

int main(int argc, char *argv[])
{

    if (argc < 2) {
        printf("usage:  %s [address]\n", argv[0]);
        return EXIT_SUCCESS;
    }

    init_enet_client(argv[1], PORT);
    init_sdl(LAND_WIDTH, LAND_HEIGHT, "MOAG");

    if (!set_font("Nouveau_IBM.ttf", 14))
        DIE("Failed to open 'Nouveau_IBM.ttf'\n");

    struct moag moag;

    memset(&moag, 0, sizeof(moag));

    ENetEvent enet_ev;

    while (!is_closed()) {

        grab_events();

        if (typing_str && is_text_input()) {
            if (is_key_down(SDLK_ESCAPE)
                || is_key_down(SDLK_LEFT)
                || is_key_down(SDLK_RIGHT)
                || is_key_down(SDLK_UP)
                || is_key_down(SDLK_DOWN)) {
                typing_str=NULL;
                stop_text_input();
            }
            else if(is_key_down(SDLK_RETURN)) {
                unsigned char buffer[256];
                size_t pos = 0;

                unsigned char len = strlen(typing_str);
                write8(buffer, &pos, CLIENT_MSG_CHUNK);
                write8(buffer, &pos, len);
                for (int i = 0; i < len; ++i)
                    write8(buffer, &pos, typing_str[i]);

                send_packet(buffer, pos, true);

                stop_text_input();
                typing_str = NULL;
                stop_text_input();
            }
        }
        else {
            if (is_key_down(SDLK_LEFT) && !kleft) {
                send_byte(KLEFT_PRESSED_CHUNK);
                kleft = true;
            }
            else if (!is_key_down(SDLK_LEFT) && kleft) {
                send_byte(KLEFT_RELEASED_CHUNK);
                kleft = false;
            }

            if (is_key_down(SDLK_RIGHT) && !kright) {
                send_byte(KRIGHT_PRESSED_CHUNK);
                kright = true;
            }
            else if (!is_key_down(SDLK_RIGHT) && kright) {
                send_byte(KRIGHT_RELEASED_CHUNK);
                kright = false;
            }

            if (is_key_down(SDLK_UP) && !kup) {
                send_byte(KUP_PRESSED_CHUNK);
                kup = true;
            }
            else if (!is_key_down(SDLK_UP) && kup) {
                send_byte(KUP_RELEASED_CHUNK);
                kup = false;
            }

            if (is_key_down(SDLK_DOWN) && !kdown) {
                send_byte(KDOWN_PRESSED_CHUNK);
                kdown = true;
            }
            else if (!is_key_down(SDLK_DOWN) && kdown) {
                send_byte(KDOWN_RELEASED_CHUNK);
                kdown = false;
            }

            if (is_key_down(' ') && !kfire) {
                send_byte(KFIRE_PRESSED_CHUNK);
                kfire = true;
            }
            else if (!is_key_down(' ') && kfire) {
                send_byte(KFIRE_RELEASED_CHUNK);
                kfire = false;
            }

            if(is_key_down('t')){
                typing_str=start_text_input();
            }
            if(is_key_down('/')){
                typing_str=start_text_cmd_input();
            }
        }

        while (enet_host_service(get_client_host(), &enet_ev, 10)) {
            switch (enet_ev.type) {
            case ENET_EVENT_TYPE_CONNECT:
                break;

            case ENET_EVENT_TYPE_DISCONNECT:
                INFO("Disconnected from server.\n");
                close_window();
                break;

            case ENET_EVENT_TYPE_RECEIVE:
                on_receive(&moag, &enet_ev);
                enet_packet_destroy(enet_ev.packet);
                break;

            default:
                break;
            }
        }

        SDL_FillRect(SDL_GetVideoSurface(), NULL, COLOR_BLACK);
        draw(&moag);
        char buf[256];
        sprintf(buf, "%u", get_peer()->roundTripTime);
        draw_string_right(LAND_WIDTH, 0, COLOR_GREEN, buf);
        SDL_Flip(SDL_GetVideoSurface());

        struct timespec t;
        t.tv_sec = 0;
        t.tv_nsec = 10000000;
        while (nanosleep(&t, &t) == -1)
            continue;
    }

    uninit_sdl();
    uninit_enet();

    exit(EXIT_SUCCESS);
}
