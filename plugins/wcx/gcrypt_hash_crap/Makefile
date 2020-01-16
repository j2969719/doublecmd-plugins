CC = gcc
CFLAGS = -shared -fPIC -Wl,--no-as-needed -std=gnu99
INCLUDES = -lpcre -lgcrypt -lgpg-error -I../../sdk `pkg-config --cflags --libs glib-2.0`
PLUGNAME = gcrypt_hash_crap.wcx

all:
		$(CC) $(CFLAGS) $(INCLUDES) plugin.c -o $(PLUGNAME)

clean:
		$(RM) $(PLUGNAME)
