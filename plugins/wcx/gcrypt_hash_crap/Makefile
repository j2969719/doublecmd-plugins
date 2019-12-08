CC = gcc
CFLAGS = -shared -fPIC -Wl,--no-as-needed
INCLUDES = -lpcre -lgcrypt -lgpg-error -I../../sdk
PLUGNAME = gcrypt_hash_crap.wcx

all:
		$(CC) $(CFLAGS) $(INCLUDES) plugin.c -o $(PLUGNAME)

clean:
		$(RM) $(PLUGNAME)
