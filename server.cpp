#include "moag.h"
#include <unistd.h>
#include <math.h>

const int MAX_CLIENTS = 8;
const int MAX_BULLETS = 256;
const float GRAVITY = 0.1;

const int WIDTH  = 800;
const int HEIGHT = 600;

const int LAND_CHUNK   = 1;
const int TANK_CHUNK   = 2;
const int BULLET_CHUNK = 3;
const int MSG_CHUNK    = 4;

void disconnect_client(int);

moag::Connection clients[MAX_CLIENTS] = {};
int numClients = 0;

char land[WIDTH*HEIGHT];
Tank tanks[MAX_CLIENTS];
Bullet bullets[MAX_BULLETS];
int bulletsFired;
int spawns;

inline char landAt(int x, int y){
    if(x<0 || x>=WIDTH || y<0 || y>=HEIGHT)
        return -1;
    return land[y*WIDTH+x];
}

inline void setLandAt(int x, int y, char to) {
    if(x<0 || x>=WIDTH || y<0 || y>=HEIGHT)
        return;
    land[y*WIDTH+x] = to;
}

void spawnTank(int id){
    spawns++;
    sprintf(tanks[id].name,"p%d",id);
    tanks[id].active=1;
    tanks[id].x=(spawns*240)%(WIDTH-40)+20;
    tanks[id].y=60;
    tanks[id].angle=35;
    tanks[id].power=0;
    tanks[id].bullet=1;
    tanks[id].facingLeft=0;
    tanks[id].kLeft=0;
    tanks[id].kRight=0;
    tanks[id].kUp=0;
    tanks[id].kDown=0;
    tanks[id].kFire=0;
}

void sendChat(int to, int id, char cmd, const char* msg, unsigned char len){
    if(to<-1 || to>=MAX_CLIENTS || id<0 || id>=MAX_CLIENTS || !clients[id])
        return;
    
    // Prepare chunk.
    moag::ChunkEnqueue8(MSG_CHUNK);
    moag::ChunkEnqueue8(id);
    moag::ChunkEnqueue8(cmd);
    moag::ChunkEnqueue8(len);
    for(int i=0;i<len;i++)
        moag::ChunkEnqueue8(msg[i]);
    
    // Send chunk.
    if(to!=-1){
        if(clients[to] && moag::SendChunk(clients[to], -1, 1)==-1)
            disconnect_client(to);
    }else{
        for(int i=0;i<MAX_CLIENTS;i++)
            if(clients[i] && moag::SendChunk(clients[i], -1, 0)==-1)
                disconnect_client(i);
    }
    
    // Clear the queue buffer.
    moag::SendChunk(NULL, -1, 1);
}

void sendLand(int to, int x, int y, int w, int h){
    if(to<-1 || to>=MAX_CLIENTS)
        return;
    if(x<0){ w+=x; x=0; }
    if(y<0){ h+=y; y=0; }
    if(x+w>WIDTH) w=WIDTH-x;
    if(y+h>HEIGHT) h=HEIGHT-y;
    if(w<=0 || h<=0 || x+w>WIDTH || y+h>HEIGHT)
        return;
    
    // Prepare chunk.
    moag::ChunkEnqueue8(LAND_CHUNK);
    moag::ChunkEnqueue16(x);
    moag::ChunkEnqueue16(y);
    moag::ChunkEnqueue16(w);
    moag::ChunkEnqueue16(h);
    
    int count = 0;
    for (int yy = y; yy < h + y; ++yy)
        for (int xx = x; xx < w + x; ++xx) {
            moag::ChunkEnqueue8(landAt(xx, yy));
            count++;
        }
    fflush(stdout);

    // Send chunk.
    if(to!=-1){
        if(clients[to] && moag::SendChunk(clients[to], -1, 1)==-1)
            disconnect_client(to);
    }else{
        for(int i=0;i<MAX_CLIENTS;i++)
            if(clients[i] && moag::SendChunk(clients[i], -1, 0)==-1)
                disconnect_client(i);
    }
    
    // Clear the queue buffer.
    moag::SendChunk(NULL, -1, 1);
}

void sendTank(int to, int id) {
    if(to<-1 || to>=MAX_CLIENTS || (to>=0 && !clients[to]) || id<0 || id>=MAX_CLIENTS)
        return;

    moag::ChunkEnqueue8(TANK_CHUNK);
    moag::ChunkEnqueue8(id);
    
    if(!tanks[id].active){
        tanks[id].x=-1;
        tanks[id].y=-1;
    }
    
    moag::ChunkEnqueue16(tanks[id].x);
    moag::ChunkEnqueue16(tanks[id].y);
    if (tanks[id].facingLeft)
        moag::ChunkEnqueue8(-tanks[id].angle);
    else
        moag::ChunkEnqueue8(tanks[id].angle);

    if (to != -1) {
        if (moag::SendChunk(clients[to], -1, 1) == -1)
            disconnect_client(to);
    } else {
        for (int i = 0; i < MAX_CLIENTS; i++)
            if (clients[i] && moag::SendChunk(clients[i], -1, 0) == -1)
                disconnect_client(i);
    }
    
    // Clear the queue buffer.
    moag::SendChunk(NULL, -1, 1);
}

void sendBullets() {
    int count = 0;
    for (int i = 0; i < MAX_BULLETS; i++)
        if (bullets[i].active)
            count++;
    
    // Queue the bytes.
    moag::ChunkEnqueue8(BULLET_CHUNK);
    moag::ChunkEnqueue16(count);

    for (int i = 0; i < MAX_BULLETS; i++)
        if (bullets[i].active) {
            moag::ChunkEnqueue16(bullets[i].x);
            moag::ChunkEnqueue16(bullets[i].y);
        }

    // Send the bytes.
    for (int i = 0; i < MAX_CLIENTS; i++)
        if (clients[i] && moag::SendChunk(clients[i], -1, 0) == -1)
            disconnect_client(i);
    
    // Clear the queue.
    moag::SendChunk(NULL, -1, 1);
}

void spawnClient(int id){
    spawnTank(id);
    sendLand(id,0,0,WIDTH,HEIGHT);
    sendTank(id,-1);
    sendChat(-1,id,2,tanks[id].name,strlen(tanks[id].name));
    for(int i=0;i<MAX_CLIENTS;i++)
        if(i!=id && clients[i])
            sendChat(id,i,2,tanks[i].name,strlen(tanks[i].name));
}

void disconnect_client(int c){
    printf("Client DCed\n");
    moag::Disconnect(clients[c]);
    numClients--;
    clients[c]=NULL;
    tanks[c].active=0;
    sendTank(-1,c);
}



void fireBullet(char type, int x, int y, float angle, float vel){
    int i=0;
    while(bullets[i].active)
        if(++i>=MAX_BULLETS)
            return;
    bullets[i].active=4;
    bullets[i].type=type;
    bullets[i].x=x;
    bullets[i].y=y;
    bullets[i].fx=(float)x;
    bullets[i].fy=(float)y;
    bullets[i].vx=vel*cosf(angle*M_PI/180.0);
    bullets[i].vy=-vel*sinf(angle*M_PI/180.0);
}

inline int sqr(int n){ return n*n; }

void explode(int x, int y, int rad, char type){
    // type: 0=explode, 1=dirt, 2=clear dirt
    char p=type==1?1:0;
    for(int iy=-rad;iy<=rad;iy++)
    for(int ix=-rad;ix<=rad;ix++)
        if(ix*ix+iy*iy<rad*rad)
            setLandAt(x+ix,y+iy, p);
    if(type==0)
        for(int i=0;i<MAX_CLIENTS;i++)
            if(tanks[i].active && sqr(tanks[i].x-x)+sqr(tanks[i].y-3-y)<sqr(rad+6))
                spawnTank(i);
            
    sendLand(-1,x-rad,y-rad,rad*2,rad*2);
}

void tankUpdate(int id){
    Tank& t=tanks[id];
    if(!t.active)
        return;
    // Movement
    if(t.kLeft){
        t.facingLeft=1;
        for(int i=0;i<8;i++)
            if(landAt(t.x-1,t.y-i)==0){
                if((t.x--)<=10)
                    t.x=10;
                t.y-=i;
                break;
            }
    }else if(t.kRight){
        t.facingLeft=0;
        for(int i=0;i<8;i++)
            if(landAt(t.x+1,t.y-i)==0){
                if((t.x++)>=WIDTH-10)
                    t.x=WIDTH-10;
                t.y-=i;
                break;
            }
    }
    // Aim
    if(t.kUp && t.angle<90)
        t.angle++;
    else if(t.kDown && t.angle>1)
        t.angle--;
    // Fire
    if(t.kFire){
        if(t.power<1000)
            t.power+=10;
    }else if(t.power){
        if((++bulletsFired)%50==0)
            t.bullet=3;
        else if(bulletsFired%313==0)
            t.bullet=2;
        fireBullet(t.bullet, t.x,t.y-7,t.facingLeft?180-t.angle:t.angle, (float)t.power*0.01);
        t.bullet=1;
        t.power=0;
    }
    // Physics
    if(landAt(t.x,t.y+1)==0)
        t.y++;
    if(landAt(t.x,t.y+1)==0)
        t.y++;
    sendTank(-1,id);
}

void bulletDetonate(int b){
    switch(bullets[b].type){
    case 1: // missile
        explode(bullets[b].x,bullets[b].y, 12, 0);
        break;
    case 2: // nuke
        explode(bullets[b].x,bullets[b].y, 128, 0);
        break;
    case 3: // dirt
        explode(bullets[b].x,bullets[b].y, 64, 1);
        break;
    default: break;
    }
    bullets[b].active=0;
}

void bulletUpdate(int b){
    if(!bullets[b].active)
        return;
    bullets[b].fx+=bullets[b].vx;
    bullets[b].fy+=bullets[b].vy;
    bullets[b].vy+=GRAVITY;
    bullets[b].x=(int)bullets[b].fx;
    bullets[b].y=(int)bullets[b].fy;
    if(landAt(bullets[b].x,bullets[b].y)){
        bulletDetonate(b);
        return;
    }
    if(bullets[b].active>1){
        bullets[b].active--;
        return;
    }
    for(int i=0;i<MAX_CLIENTS;i++)
        if(sqr(tanks[i].x-bullets[b].x)+sqr(tanks[i].y-3-bullets[b].y)<72){
            bulletDetonate(b);
            break;
        }    
}

void initGame(){
    bulletsFired=0;
    spawns=0;
    for(int i=0;i<MAX_CLIENTS;i++)
        tanks[i].active=0;
    for(int i=0;i<MAX_BULLETS;i++)
        bullets[i].active=0;
        
    for (int y = 0; y < HEIGHT; ++y)
        for (int x = 0; x < WIDTH; ++x){
            if (y < HEIGHT / 2)
                setLandAt(x, y, 0);
            else
                setLandAt(x, y, 1);
        }
    sendLand(-1,0,0,WIDTH,HEIGHT);
}

void stepGame(){
    for(int i=0;i<MAX_CLIENTS;i++)
        tankUpdate(i);
    for(int i=0;i<MAX_BULLETS;i++)
        bulletUpdate(i);
    sendBullets();
}



void client_connect(moag::Connection arg) {
    int i=0;
    while(clients[i])
        if(++i>=MAX_CLIENTS){
            printf("Client failed to connect, too many clients.\n");
            moag::Disconnect(arg);
            return;
        }
    
    clients[i] = arg;
    numClients += 1;
    printf("Client connected!\n");
    spawnClient(i);
    
    fflush(stdout);
}

void handleMsg(int id, const char* msg, int len){
    if(msg[0]=='/' && msg[1]=='n' && msg[2]==' '){
        len-=3;
        if(len>15)
            len=15;
        for(int i=0;i<len;i++)
            tanks[id].name[i]=msg[i+3];
        tanks[id].name[len]='\0';
        sendChat(-1,id,2,tanks[id].name,strlen(tanks[id].name));
        return;
    }
    sendChat(-1,id,1,msg,len);
}

void server_update(moag::Connection arg) {
    stepGame();

    // For each client...
    for (int i = 0; i < MAX_CLIENTS; ++i)
        while (clients[i] && moag::HasActivity(clients[i], 0)) {
            // Get data.
            if(moag::ReceiveChunk(clients[i], 1)==-1){
                disconnect_client(i);
                continue;
            }
            
            char byte = moag::ChunkDequeue8();

            switch(byte){
            case 1: tanks[i].kLeft=1; break;
            case 2: tanks[i].kLeft=0; break;
            case 3: tanks[i].kRight=1; break;
            case 4: tanks[i].kRight=0; break;
            case 5: tanks[i].kUp=1; break;
            case 6: tanks[i].kUp=0; break;
            case 7: tanks[i].kDown=1; break;
            case 8: tanks[i].kDown=0; break;
            case 9: tanks[i].kFire=1; break;
            case 10: tanks[i].kFire=0; break;
            case 11: { //msg
                if(moag::ReceiveChunk(clients[i], 1)==-1)
                    break;
                unsigned char len = moag::ChunkDequeue8();
                char* msg=new char[len];
                if(moag::ReceiveChunk(clients[i], len)==-1)
                    break;
                for(int j=0;j<len;j++)
                    msg[j] = moag::ChunkDequeue8();
                handleMsg(i,msg,len);
                delete[] msg;
            } break;
            default: break;
            }
        }

    if (numClients == 0)
        usleep(200000);
    else
        usleep(20000);

    fflush(stdout);
}

int main(int argc, char *argv[])
{
    if (moag::OpenServer(8080) == -1) {
        printf("Failed to start server\n");
        return 1;
    }
    
    moag::SetServerCallback(client_connect, moag::CB_CLIENT_CONNECT);
    moag::SetServerCallback(server_update, moag::CB_SERVER_UPDATE);
    
    initGame();
    printf("Started server\n");

    while (1)
        moag::ServerTick();
    
    moag::CloseServer();

    return 0;
}

