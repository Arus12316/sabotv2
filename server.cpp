#include "server.h"
#include "user.h"

const char Server::KEY_2D[] = "2D Central";
const char Server::KEY_PTC[] = "Paper Thin City";
const char Server::KEY_FLI[] = "Fine Line Island";
const char Server::KEY_USA[] = "U of SA";
const char Server::KEY_FW[] = "Flat World";
const char Server::KEY_POU[] = "Planar Outpost";
const char Server::KEY_MMET[] = "Mobius Metropolis";
const char Server::KEY_EU[] = "Eu Amsterdam";
const char Server::KEY_COMP[] = "Compatibility";
const char Server::KEY_SS[] = "SS Lineage";

const char Server::S_REGISTER[] = "67.19.145.10";
const char Server::S_2D_CENTRAL[] = "74.86.43.9";
const char Server::S_PAPER_THIN[] = "74.86.43.8";
const char Server::S_FINE_LINE[] = "67.19.138.234";
const char Server::S_U_OF_SA[] = "67.19.138.236";
const char Server::S_FLAT_WORLD[] = "74.86.3.220";
const char Server::S_PLANAR_OUTPOST[] = "67.19.138.235";
const char Server::S_MOBIUS_METROPOLIS[] = "74.86.3.221";
const char Server::S_AMSTERDAM[] = "94.75.214.10";
const char Server::S_COMPATABILITY[] = "74.86.3.222";
const char Server::S_SS_LINEAGE[] = "74.86.43.10";

Server *Server::servers[N_GAMESERVERS];

const char *Server::saServers[][2] = {
    {KEY_2D, S_2D_CENTRAL},
    {KEY_PTC, S_PAPER_THIN},
    {KEY_FLI, S_FINE_LINE},
    {KEY_USA, S_U_OF_SA},
    {KEY_FW, S_FLAT_WORLD},
    {KEY_POU, S_PLANAR_OUTPOST},
    {KEY_MMET, S_MOBIUS_METROPOLIS},
    {KEY_EU, S_AMSTERDAM},
    {KEY_COMP, S_COMPATABILITY},
    {KEY_SS, S_SS_LINEAGE}
};

Server::Server(int index, QObject *parent) :
    QObject(parent)
{
    this->index = index;
    this->master = NULL;


    for(int i = 0; i < UID_TABLE_SIZE; i++)
        utable[i] = NULL;
}

const char *Server::getIP()
{
    return Server::saServers[this->index][1];
}

const char *Server::getName()
{
    printf("%d\n", this->index);
    printf("%p\n", Server::saServers[this->index]);

    fflush(stdout);
    return Server::saServers[this->index][0];
}

const char *Server::toIP(const char key[])
{
    for(int i = 0; i < N_GAMESERVERS; i++) {
        if(!strcmp(Server::saServers[i][0], key)) {
            return Server::saServers[i][1];
        }
    }
    return NULL;
}

quint16 Server::hashUid(const char *uid)
{
    quint32 h = 0, g;

    h = (h << 4) + uid[0];
    if((g = h & 0xf0000000)) {
        h ^= (g >> 24);
        h ^= g;
    }
    h = (h << 4) + uid[1];
    if((g = h & 0xf0000000)) {
        h ^= (g >> 24);
        h ^= g;
    }
    h = (h << 4) + uid[2];
    if((g = h & 0xf0000000)) {
        h ^= (g >> 24);
        h ^= g;
    }
    return h % UID_TABLE_SIZE;
}

void Server::insertUser(User *u)
{
    char *key = u->id;
    hash_s **prec = &utable[hashUid(key)],
            *rec = *prec,
            *n = new hash_s;

    n->next = rec;
    n->user = u;
    *prec = n;
}

void Server::deleteUser(const char *key)
{
    hash_s **prec = &utable[hashUid(key)],
            *rec = *prec, *last = NULL;

    while(rec) {
        if(!strcmp(rec->user->id, key)) {
            if(last) {
                last->next = rec->next;
            }
            else {
                *prec = rec->next;
            }
            delete rec;
            break;
        }
        last = rec;
        rec = rec->next;
    }
}

User *Server::lookupUser(const char *key)
{
    hash_s *rec = utable[hashUid(key)];

    while(rec) {
        if(!strcmp(rec->user->id, key))
            return rec->user;
        rec = rec->next;
    }
    qDebug() << "Error: Tried to look up " << key;

    return NULL;
}
