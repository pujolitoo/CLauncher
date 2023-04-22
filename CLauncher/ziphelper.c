#include "ziphelper.h"
#include "osutil.h"
#include "strutils.h"
#include "allocator.h"
#include <crtdbg.h>

int ExtractAll(const char* zipfile, const char* path)
{
	zip_error_t errCode;
	char errbuf[100];
	zip_stat_t currFileInfo;
	zip_file_t* currFile;
	void* databuffer;
	HeapString currPath;

	FILE* outFile;

	zip_t* zipFile = zip_open(zipfile, ZIP_RDONLY, &errCode);
	if (zipFile == NULL)
	{
		const char* errstr = zip_error_strerror(&errCode);
		fprintf(stderr, "ZipFile failed: %s\n", errstr);
		return 1;
	}

	zip_int64_t numEntries = zip_get_num_entries(zipfile, 0);

	for (int i = 0; i < numEntries; i++)
	{
		zip_stat_index(zipFile, i, 0, &currFileInfo);

		//IF FOLDER
		if (currFileInfo.name[strlen(currFileInfo.name) - 1] == '/')
		{
			currPath = join(path, "/", currFileInfo.name, NULL);
			NORMALIZE(currPath);
			RECDIRS(currPath.string);
			cleanString(&currPath, false);
		}
		//IF FILE
		else
		{
			currPath = join(path, "/", currFileInfo.name, NULL);
			currFile = zip_fopen_index(zipFile, i, 0);

			if (currFile != NULL)
			{
				databuffer = xmalloc(currFileInfo.size);

				if (fopen_s(&outFile, currPath.string, "wb") != 0)
				{
					fprintf(stderr, "Error writing to file.\n");
					return 2;
				}
				zip_fread(currFile, databuffer, currFileInfo.size);

				fwrite(databuffer, currFileInfo.size, 1, outFile);

				fclose(outFile);
				zip_fclose(currFile);

				xfree(databuffer);

				cleanString(&currPath, false);
			}
			else
			{
				break;
			}
		}
	}

	zip_close(zipFile);
	return 0;
}