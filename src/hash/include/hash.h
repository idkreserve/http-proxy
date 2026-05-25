#ifndef HASH_H
#define HASH_H

struct nlist {
    struct nlist *next;
    char *value;
};

#define HASHSIZE 101

struct nlist *lookup(char *s);
struct nlist *install(char *value);

#endif
