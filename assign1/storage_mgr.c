#ifndef STORAGE_MGR_H
#define STORAGE_MGR_H

#include "stdlib.h"
#include "dberror.h"
#include "storage_mgr.h"
/*initialize the data structure,
  cause there is no global variable
  then we do nothing.*/
void initStorageManager(void)
{
}

RC createPageFile(char *fileName)
{
		fileName = (char *) malloc(PAGE_SIZE);
		for(int i = 0; i <PAGE_SIZE; i++){
				fileName[i] = 0;
		}
		return RC_OK;
}

RC openPageFile(char *fileName, SM_FileHandle *fHandle)
{
		if(!fileName)
				return RC_FILE_NOT_FOUND;					
		else
				fHandle.fileName = fileName;
				fHandle.curPagePos = 0;
				fHandle.totalNumPages = 1;
				fHandle.mgmtInfo = 1;
				return RC_OK;
}

RC closePageFile(SM_FileHandle *fHandle)
{
		free(fHandle.mgmtInfo);
		fHandle.mgmtInfo = NULL;
}

RC destroyPageFile(char *fileName)
{
		free(fileName);
		fileName = NULL;
}
