#include <stdlib.h>
#include <string.h>
#include "hash.h"

static struct nlist *hashtab[HASHSIZE];

static unsigned hash(char *s);

static unsigned hash(char *s)
{
  unsigned hashval = 0;

  while (*s != '\0')
    hashval = *s++ + 31 * hashval;
  
  return hashval % (sizeof hashtab / sizeof *hashtab);
}

struct nlist *lookup(char *s)
{
  for (struct nlist *np = hashtab[hash(s)]; np != NULL; np = np->next)
    if (strcmp(s, np->value) == 0)
      return np;
  return NULL;
}

struct nlist *install(char *value)
{
  struct nlist *np;

  if ((np = lookup(value)) == NULL) {
    np = malloc(sizeof *np);
    const unsigned hashval = hash(value);
    np->next = hashtab[hashval];
    hashtab[hashval] = np;
  } else
    free(np->value);
  if ((np->value = strdup(value)) == NULL)
    return NULL;
  return np;
}
