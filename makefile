CC = gcc

# Options du compilateur
CFLAGS = -std=c2x -g -D_XOPEN_SOURCE -Wpedantic -Wall -Wextra -Wconversion -Werror -fstack-protector-all -fpie -pie -O2 -D_FORTIFY_SOURCE=2 -MMD
#-fsanitize=address -g
#LDFLAGS = -fsanitize=address
# Noms des exécutables
TARGETS = client server

# Fichiers source pour chaque cible
SRCS_client = client.c semaphore2.c
SRCS_server = server.c semaphore2.c

# Génération des fichiers objets correspondants
OBJS_client = $(SRCS_client:.c=.o)
OBJS_server = $(SRCS_server:.c=.o)

# Dépendances automatiques
DEPS_client = $(OBJS_client:.o=.d)
DEPS_server = $(OBJS_server:.o=.d)

# Cible par défaut
all: $(TARGETS)

# Règles pour chaque exécutable
client: $(OBJS_client)
	$(CC) $(CFLAGS) -o $@ $^

server: $(OBJS_server)
	$(CC) $(CFLAGS) -o $@ $^

# Règle générique pour compiler les fichiers source en fichiers objets
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Inclure les fichiers de dépendances
-include $(DEPS_client)
-include $(DEPS_server)

# Cibles de nettoyage
clean:
	rm -f $(OBJS_client) $(OBJS_server) $(DEPS_client) $(DEPS_server)

distclean: clean
	rm -f $(TARGETS)

# Déclarer les cibles phony
.PHONY: all clean distclean
