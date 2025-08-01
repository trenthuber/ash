#include <err.h>
#include <fcntl.h>
#include <dirent.h>
#include <string.h>
#include <sys/errno.h>

#include "../../external/cbs/cbs.c"
#include "../../external/cbsfile.c"
#include "../config.h"

int main(void) {
	int listfd, l;
	DIR *dir;
	size_t i, offset;
	struct cbsfile files[1 + MAXBUILTINS + 1];
	struct dirent *entry;
	char *name, *identifier;

	build("./");

	if ((listfd = open("list.c", O_WRONLY | O_CREAT | O_TRUNC, 0644)) == -1)
		err(EXIT_FAILURE, "Unable to open/create `list.c'");
	if (!(dir = opendir("./")))
		err(EXIT_FAILURE, "Unable to open current directory");

	dprintf(listfd, "#include <stdlib.h>\n\n#include \"builtin.h\"\n"
	        "#include \"list.h\"\n\n");

	errno = i = 0;
	files[i++] = (struct cbsfile){"../builtin", NONE, 's'};
	files[i++] = (struct cbsfile){"list", NONE};
	offset = i;
	while ((entry = readdir(dir))) {
		if (strcmp(entry->d_name, "build.c") == 0) continue;
		if (!(name = strrchr(entry->d_name, '.')) || strcmp(name, ".c") != 0)
			continue;
		if (i == 1 + MAXBUILTINS + 1)
			errx(EXIT_FAILURE, "Maximum allowed builtins (%d) exceeded", MAXBUILTINS);
		if (!(name = strdup(entry->d_name)))
			err(EXIT_FAILURE, "Unable to duplicate directory entry");
		name[strlen(name) - strlen(".c")] = '\0';
		if (strcmp(name, "list") == 0) continue;
		if (strcmp(name, "builtin") != 0)
			dprintf(listfd, "extern BUILTINSIG(%s);\n", name);
		files[i++] = (struct cbsfile){name, LIST("-I../")};
	}
	if (errno) err(EXIT_FAILURE, "Unable to read from current directory");
	files[i] = (struct cbsfile){NULL};
	
	identifier = "struct builtin builtins[] = {";
	l = (int)strlen(identifier);
	dprintf(listfd, "\n%s", identifier);
	for (i = offset; (name = files[i].name); ++i)
		if (strcmp(name, "builtin") != 0 && strcmp(name, "list") != 0)
			dprintf(listfd, "{\"%s\", %s},\n%*s", name, name, l, "");
	dprintf(listfd, "{NULL}};");

	if (closedir(dir) == -1)
		err(EXIT_FAILURE, "Unable to close current directory");
	if (close(listfd) == -1) err(EXIT_FAILURE, "Unable to close `list.c'");

	buildfiles(files);

	while (files[offset].name) free(files[offset++].name);

	return EXIT_SUCCESS;
}
