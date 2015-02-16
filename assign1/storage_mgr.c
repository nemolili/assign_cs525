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
		return RC_OK;
}

RC destroyPageFile(char *fileName)
{
		free(fileName);
		fileName = NULL;
		return RC_OK;
}

RC readBlock(int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage)
{
		readStartPos = (pageNum-1)*PAGE_SIZE;
		if(pageNum > fHandle.totalNumPages)
				return RC_READ_NON_EXISTING_PAGE;
		else 
				for(int i=0; i< PAGE_SIZE; i++)
						memPage[i] = fHandle.fileName[readStartPos+i];
				return RC_OK;

}

int getBlockPos(SM_FileHandle *fHandle)
{
		return fHandle.curPagePos;
}

RC readFirstBlock(SM_FileHandle *fHandle, SM_PageHandle memPage)
{
		for(int i=0; i<PAGE_SIZE; i++)
				memPage[i] = fHandle.fileName[i];
		return RC_OK;
}

RC readPreviousBlock(SM_FileHandle *fHandle, SM_PageHandle memPage)
{
		int readStartPos = (fHandle.curPagePos-2)*PAGE_SIZE;
		for(int i=0; i<PAGE_SIZE; i++)
				memPage[i] = fHandle.fileName[readStartPos + i];
		return RC_OK;
}

RC readCurrentBlock(SM_FileHandle *fHandle, SM_PageHandle memPage)
{
		int readStartPos = (fHandle.curPagePos-1)*PAGE_SIZE;
		for(int i=0; i<PAGE_SIZE; i++)
				memPage[i] = fHandle.fileName[readStartPos + i];
		return RC_OK;
}

RC readNextBlock(SM_FileHandle *fHandle, SM_PageHandle memPage)
{
		if(fHandle.curPagePos == fHandle.totalNumPages)
				return RC_READ_NON_EXISTING_PAGE;
		else
				int readStartPos = (fHandle.curPagePos-1)*PAGE_SIZE;
				for(int i=0; i<PAGE_SIZE; i++)
						memPage[i] = fHandle.fileName[readStartPos + i];
				return RC_OK;
}

RC readLastBlock(SM_FileHandle *fHandle, SM_PageHandle memPage)
{
		int readStartPos = (fHandle.totalNumPages-1)*PAGE_SIZE;
		for(int i=0; i<PAGE_SIZE; i++)
				memPage[i] = fHandle.fileName[readStartPos + i];
		return RC_OK;
}

RC writeBlock(int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage)
{
		readStartPos = (pageNum-1)*PAGE_SIZE;
		if(pageNum > fHandle.totalNumPages)
				return RC_READ_NON_EXISTING_PAGE;
		else 
				for(int i=0; i< PAGE_SIZE; i++)
						fHandle.fileName[readStartPos + i] = memPage[i];
				return RC_OK;
}

RC writeCurrentBlock(SM_FileHandle *fHandle, SM_PageHandle memPage)
{
		int readStartPos = (fHandle.curPagePos-1)*PAGE_SIZE;
		for(int i=0; i<PAGE_SIZE; i++)
				fHandle.fileName[readStartPos + i] = memPage[i];
		return RC_OK;
}

RC appendEmptyBlock(SM_FileHandle *fHandle)
{
		int readStartPos = (fHandle.totalNumPage)*PAGE_SIZE;
		fHandle.totalNumPages += 1; 
		for(int i=0; i<PAGE_SIZE; i++)
				fHandle.fileName[readStartPos + i] = 0;
		return RC_OK;
}

RC ensureCapacity(int numberOfPages, SM_FileHandle *fHandle)
{
		numPageAdd = numberOfPages - fHandle.totalNumPages;
		for(int i=0; i<numPageAdd; i++)
				fHandle.fileName[fHandle.curPagePos * PAGE_SZIE+i] = 0;
		return RC_OK
}
