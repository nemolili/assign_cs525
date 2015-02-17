#include <stdio.h>
#include <stdlib.h>
#include "dberror.h"
#include "storage_mgr.h"


/*********************************************************
 *	Initialize the storage manager and output success    *
 *                                                       *
 ********************************************************/
void initStorageManager(void)
{
		printf("The storage Manager are initialize");
} 


/*********************************************************
 *   <<Interface---createPageFile>>                      *
 *	create a new pagefile and return RC_OK               *
 *                                                       *
 ********************************************************/
RC createPageFile(char *fileName)
{
		FILE* fc = fopen(fileName,"w+");       //create a file stream 
		char* content = (char *) malloc(PAGE_SIZE);  //create a buffer and fill the buffer with "\0" bytes;
		for(int i=0; i<PAGE_SIZE;i++){
				content[i] = 0;
		}
		if(fwrite(content,sizeof(char),PAGE_SIZE,fc) == 0)   //write the content from the buffer into the file stream;
		{   
				return RC_WRITE_FAILED;
		}
		else
		{
				fclose(fc);
				fc = NULL;
				free(content);              //free the memory;
				content = NULL;
				return RC_OK;
		}
}

/***************************************************************
 *   <<Interface---openPageFile>>                              *
 *	open the page file and connect the file to the FILE HANDLE *
 *                                                             *
 ***************************************************************/
RC openPageFile(char *fileName, SM_FileHandle *fHandle)
{

		FILE* fo = fopen(fileName,"r+");
		if(!fo)
		{
				return RC_FILE_NOT_FOUND;					
		}
		else
		{         //Initialize the FILE_HANDLE;
				if(!fHandle)
				{
						return RC_FILE_HANDLE_NOT_INIT;
				}
				else
				{
						fHandle->fileName = fileName;
						fHandle->curPagePos = 0;  //set the first page 0;
						fHandle->totalNumPages = sizeof(fo)/PAGE_SIZE+1; 
						fHandle->mgmtInfo = fo;    //mgmtInfo store the pointer to the file stream;
						return RC_OK;
				}
		}
}


/***********************************************************************
 *   << Interface---closePageFIle >>                                   *
 *  close the open page file and free the pointer, mgmtInfo, stored in * 
 *  the mgmtInfo in the FILE HANDLE                                    *
 *                                                                     *
 ***********************************************************************/
RC closePageFile(SM_FileHandle *fHandle)
{
		if(!fHandle->mgmtInfo)  // the page file did not open
		{
				printf("The page file did not open;");
				return RC_OK;
		}
		else
		{
				fclose(fHandle->mgmtInfo);
				fHandle->mgmtInfo = NULL;
				return RC_OK;
		}
}

/***************************************************
 *   << Interface---destroyPageFIle >>             *
 *  destroy the page file                          *
 *  remove the fileName                            *
 ***************************************************/
RC destroyPageFile(char *fileName)
{
		if(remove(fileName)==0)
		{
				return RC_OK;
		}
		else
		{
				return RC_DESTROY_FILE_FAILED; //create a new error message to detect the file destroyed failed;
		}
}


/**********************************************************
 *   << Interface---readBlock >>                          *
 *  Given the page number and read the pageNum th block   *
 *  Store the content to the buffer memPage page handle   *
 *                                                        *
 **********************************************************/
RC readBlock(int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage)
{
		if(pageNum > fHandle->totalNumPages) 
		{
				return RC_READ_NON_EXISTING_PAGE;
		}
		else if(!fHandle->mgmtInfo)   //if the page file is closed then open the file;
		{
			FILE* fo = fopen(fHandle->fileName,"r+");
			fHandle->mgmtInfo = fo;  //store the file stream in the fHandle;
		}
		else
		{
		}
		if(fseek(fHandle->mgmtInfo,(pageNum)*PAGE_SIZE,SEEK_SET)==0) //locate the strat position to read the content from the file;
		{
				fread(memPage,sizeof(char),PAGE_SIZE,fHandle->mgmtInfo); //read the block and store the content pointed to the memPage page handle;
				fclose(fHandle->mgmtInfo);  //After read the block, close the file stream;
				fHandle->mgmtInfo = NULL;
				return RC_OK;
		}
}


/***************************************************************
 *   << Interface---getBlockPos >>                             *
 *  return the current page position sotred in the file handle *
 *                                                             *
 ***************************************************************/
int getBlockPos(SM_FileHandle *fHandle)
{
		return fHandle->curPagePos;
}


/***************************************************************
 *   << Interface---readFirstBlock >>                          *
 *  set the pageNum 0 and call the readBlock function          *
 *                                                             *
 ***************************************************************/
RC readFirstBlock(SM_FileHandle *fHandle, SM_PageHandle memPage)
{
		readBlock(0,fHandle,memPage);
}


/***************************************************************
 *   << Interface---readPreviousBlock >>                       *
 *  set the pageNum and call the readBlock function            *
 *                                                             *
 ***************************************************************/
RC readPreviousBlock(SM_FileHandle *fHandle, SM_PageHandle memPage)
{
		readBlock((fHandle->curPagePos-2),fHandle,memPage);
}


/***************************************************************
 *   << Interface---readCurrentBlock >>                        *
 *  set the pageNum and call the readBlock function            *
 *                                                             *
 ***************************************************************/
RC readCurrentBlock(SM_FileHandle *fHandle, SM_PageHandle memPage)
{
		readBlock((fHandle->curPagePos-1),fHandle,memPage);
}


/***************************************************************
 *   << Interface---readNextBlock >>                           *
 *  set the pageNum and call the readBlock function            *
 *                                                             *
 ***************************************************************/
RC readNextBlock(SM_FileHandle *fHandle, SM_PageHandle memPage)
{
		readBlock((fHandle->curPagePos),fHandle,memPage);
}


/***************************************************************
 *   << Interface---readLastBlock >>                           *
 *  set the pageNum and call the readBlock function            *
 *                                                             *
 ***************************************************************/
RC readLastBlock(SM_FileHandle *fHandle, SM_PageHandle memPage)
{
		readBlock((fHandle->totalNumPages-1),fHandle,memPage);
}

/*********************************************************************
 *   << Interface---writeBlock >>                                    *
 *  Given the page number and wirte the content stored in the buffer *
 *  memPage page handle to the pageNumb th block                     *
 *********************************************************************/
RC writeBlock(int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage)
{
		if(pageNum > fHandle->totalNumPages)
		{
				return RC_READ_NON_EXISTING_PAGE;
		}
		else if(!fHandle->mgmtInfo)
		{
			FILE* fo = fopen(fHandle->fileName,"w+");
			fHandle->mgmtInfo = fo;
		}
		else
		{
		}
		if(fseek(fHandle->mgmtInfo,(pageNum)*PAGE_SIZE,SEEK_SET)==0)
		{
				fwrite(memPage,sizeof(char),PAGE_SIZE,fHandle->mgmtInfo);
				fclose(fHandle->mgmtInfo);
				fHandle->mgmtInfo = NULL;
				return RC_OK;
		}
}

/***************************************************************
 *   << Interface---writeCurrentBlock >>                       *
 *  set the pageNum and call the writeBlock function           *
 *                                                             *
 ***************************************************************/
RC writeCurrentBlock(SM_FileHandle *fHandle, SM_PageHandle memPage)
{
		writeBlock((fHandle->curPagePos),fHandle,memPage);
}


/***************************************************************************
 *   << Interface---appendEmptyBlock >>                                    *
 *  create a new empty block and add the block to the end of the page file *
 *                                                                         *
 ***************************************************************************/
RC appendEmptyBlock(SM_FileHandle *fHandle)
{
		if(!fHandle->mgmtInfo)   //open the closed page file;
		{
				FILE* fo = fopen(fHandle->fileName,"a+");
				fHandle->mgmtInfo = fo;
		}
		fHandle->totalNumPages += 1;   //After append empty block, the total number page increase;
		char* content = (char *) malloc(PAGE_SIZE);  //create a buffer and store the content which is wrote to the new block;
		for(int i=0; i<PAGE_SIZE;i++)
		{
				content[i] = 0;
		}
		if(fseek(fHandle->mgmtInfo,0,SEEK_END)==0)
		{
				fwrite(content,sizeof(char),PAGE_SIZE,fHandle->mgmtInfo);  //fill the empty block with "\0" bytes;
		}
		fclose(fHandle->mgmtInfo);
		fHandle->mgmtInfo = NULL;
		free(content);
		content = NULL;
		return RC_OK;
}

/***************************************************************
 *   << Interface---ensureCapacity >>                          *
 *  get the number of pages add to the page file and           *
 *  call the appendEmptyBlock function for the number times    *
 *                                                             *
 ***************************************************************/
RC ensureCapacity(int numberOfPages, SM_FileHandle *fHandle)
{
		int numPageAdd = numberOfPages - fHandle->totalNumPages;  //the nubmer of pages need to be add to the page file
		for(int i=0; i<numPageAdd; i++)
		{
				appendEmptyBlock(fHandle);
		}
}
