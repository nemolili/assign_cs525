#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "storage_mgr.h"
#include "buffer_mgr.h"

/***********************************
  struct for every single pageFlame

************************************/
typedef struct PageFlame{
	char *data;
	PageNumber pageNum;
	bool isDirty;
	int fixCount;
	struct PageFlame *next;
	struct PageFlame *prev;
} PageFlame;

/************************************************
  struct for the pointer between the bufferPool *
  and pageFlame
************************************************/
typedef struct FlameHandle{
	PageFlame *first;
	PageFlame *last;
	PageFlame *totalFlames;
	int numWriteIO;
	int numReadIO;
} FlameHandle;


/************************************************
  initialize the buffer pool,include the pointer
  struct and pageFlame struct
************************************************/
RC initBufferPool(BM_BufferPool *const bm, const char *const pageFileName,
		const int numPages, ReplacementStrategy strategy, void *stratData)
{
		PageFlame *totalPageFlames;
		totalPageFlames = (PageFlame *)malloc(sizeof(PageFlame)*numPages);
		FlameHandle *FH;
		FH = (FlameHandle *)malloc(sizeof(FlameHandle));
		bm->mgmtData = FH; 
		FH->numWriteIO = FH->numReadIO = 0;
		FH->totalFlames = totalPageFlames;
		for(int i=0; i<numPages; i++)
		{
			if(i==0)
			{
				totalPageFlames[i].prev = NULL;
			}
			else
			{
				totalPageFlames[i].prev = &totalPageFlames[i-1];
			}
			if(i==numPages-1)
			{
				totalPageFlames[i].next = NULL;
			}
			else
			{
				totalPageFlames[i].next = &totalPageFlames[i+1];
			}
			totalPageFlames[i].isDirty = false;
			totalPageFlames[i].fixCount = 0;
			totalPageFlames[i].pageNum = NO_PAGE;
			totalPageFlames[i].data = (char *)malloc(sizeof(char)*PAGE_SIZE);
		}
		FH->first = &totalPageFlames[0];
		FH->last = &totalPageFlames[numPages-1];
		bm->pageFile = (char*)pageFileName;
		bm->numPages = numPages;
		bm->strategy = strategy;
		return RC_OK;

}

/************************************************
  shut down the buffer pool 
  if some pageFlame are still used by the client
  return false;
************************************************/
RC shutdownBufferPool(BM_BufferPool *const bm)
{
	FlameHandle *FH;
	FH = bm->mgmtData;
	int numFlames = bm->numPages;
	bool existDirty = false;
	for(int m=0; m < numFlames;m++)
	{
		if(FH->totalFlames[m].fixCount != 0)
		{
			return RC_WRITE_FAILED;
		}
		if(FH->totalFlames[m].isDirty == true)
		{
			existDirty = true;
		}

	}
	if(existDirty ==  true)
	{
		forceFlushPool(bm);
	}
	free(FH);
	return RC_OK; 
}

/************************************************
  write back all dirty page to the disk
************************************************/
RC forceFlushPool(BM_BufferPool *const bm)
{
	FlameHandle *FH;
	FH = bm->mgmtData;
	SM_FileHandle fh;
	SM_PageHandle ph;
	ph = (SM_PageHandle)malloc(sizeof(SM_PageHandle));
	openPageFile(bm->pageFile,&fh);
	int numFlames = bm->numPages;
	for (int i=0;i<numFlames; i++)
	{
		PageFlame *pf = &(FH->totalFlames[i]);
		strcpy(ph,pf->data);
		if(pf->isDirty == true && pf->fixCount == 0)
		{
			ensureCapacity((pf->pageNum)+1,&fh);
			writeBlock(pf->pageNum,&fh,ph);
			pf->isDirty = false;
			FH->numWriteIO++;
		}
	}		
	return RC_OK;
}

/************************************************
  mark the page dirty and return the pageHandle
************************************************/
RC markDirty(BM_BufferPool *const bm, BM_PageHandle *const page)
{
	FlameHandle *FH;
	FH = bm->mgmtData;
	int numPages = bm->numPages;
	for(int i=0; i<numPages; i++)
	{
		PageFlame *pf = &FH->totalFlames[i];
		if( pf->pageNum== page->pageNum)
		{
			pf->isDirty = true;
			return RC_OK;
		}
	}
	return RC_OK;
}

/************************************************
  unpining the page and discrease the fixCount
************************************************/
RC unpinPage(BM_BufferPool *const bm, BM_PageHandle *const page)
{
		FlameHandle *FH;
		FH = bm->mgmtData;
		int numPages = bm->numPages;
		for(int i=0; i<numPages; i++)
		{
			PageFlame *pf = &FH->totalFlames[i];
			if(pf->pageNum == page->pageNum)
			{
				(pf->fixCount)--;
				if(pf->isDirty == true)
				{
					strcpy(pf->data,page->data);
				}
				return RC_OK;
			}
		}
		return RC_OK;
}

/************************************************
  write the current page back to disk
************************************************/
RC forcePage(BM_BufferPool *const bm, BM_PageHandle *const page)
{
	FlameHandle *FH;
	FH = bm->mgmtData;
	SM_FileHandle fh;
	SM_PageHandle ph;
	ph = (SM_PageHandle)malloc(sizeof(SM_PageHandle));
	strcpy(ph,page->data);
	openPageFile(bm->pageFile,&fh);
	writeBlock(page->pageNum,&fh,ph);
	for(int i=0;i<bm->numPages;i++)
	{
		if(FH->totalFlames[i].pageNum == page->pageNum)
		{
			FH->totalFlames[i].isDirty = false;
			break;
		}
	}

	FH->numWriteIO++;
	return RC_OK;
}

/************************************************
  pining the page with different strategy
  if the page is dirty with fixCount 0, evited the page
************************************************/
RC pinPage(BM_BufferPool *const bm, BM_PageHandle *const page, const PageNumber pageNum)
{
	FlameHandle *FH;
	FH = bm->mgmtData;
	page->data = (char*)malloc(sizeof(char)*PAGE_SIZE);
	SM_FileHandle fh;
	PageFlame *pf;
	openPageFile(bm->pageFile,&fh);
	switch(bm->strategy)
	{
		case RS_FIFO:
			page->pageNum = pageNum;
			for(int i=0; i<bm->numPages;i++)
			{
				pf = &(FH->totalFlames)[i];
				if((FH->totalFlames[i]).pageNum == pageNum)
				{
					strcpy(page->data,pf->data);
					pf->fixCount++;
					return RC_OK;;
				}
				if((FH->totalFlames[i]).pageNum == NO_PAGE)
				{
					FH->totalFlames[i].pageNum = pageNum;
					pf->fixCount++;
					ensureCapacity(pageNum+1,&fh);
					readBlock(pageNum,&fh,(FH->first)->data); 
					FH->numReadIO++;
					strcpy(page->data,(FH->first)->data);
					FH->last = FH->first;
					if(FH->last->next == NULL)
					{
						FH->first = FH->totalFlames;
					}
					else
					{
						(FH->first) = (FH->last)->next;
					}
					return RC_OK;
				}
			}	
			if((FH->first)->fixCount == 0)
			{
				if((FH->first)->isDirty == true)
				{
					ensureCapacity((FH->first)->pageNum,&fh);
					writeBlock((FH->first)->pageNum,&fh,(FH->first)->data);
					(FH->first)->isDirty = false;
					FH->numWriteIO++;	
				}
				if((FH->first)->next == NULL)
				{
					FH->first = FH->totalFlames;
				}
				else
				{
					FH->first = (FH->first)->next;
				}
				if((FH->last)->next == NULL)
				{
					FH->last = FH->totalFlames;
				}
				else
				{
					FH->last = (FH->last)->next;
				}
			}
			else
			{
				FH->last = (FH->first)->next;
				if(FH->last->isDirty==true)
				{
					ensureCapacity((FH->last)->pageNum,&fh);
					writeBlock((FH->last)->pageNum,&fh,(FH->last)->data);
					FH->numWriteIO++;	
				}
				if(((FH->first)->next)->next ==NULL)
				{
					FH->first = FH->totalFlames;
				}
				else
				{
					FH->first = (FH->first)->next;
				}
			}
			ensureCapacity(pageNum+1,&fh);
			readBlock(pageNum,&fh,page->data); 
			FH->numReadIO++;
			(FH->last)->fixCount = 1;
			(FH->last)->isDirty = false;
			(FH->last)->pageNum = pageNum;
			return RC_OK;
				
		case RS_LRU :
			for(int i=0; i<bm->numPages;i++)
			{
				pf = &FH->totalFlames[i];
				if(FH->totalFlames[i].pageNum == pageNum)
				{
					page->pageNum = FH->totalFlames[i].pageNum; 
					pf->fixCount++;
					if(FH->totalFlames[i].next != NULL)
					{
						if(FH->totalFlames[i].prev == NULL)
						{
							FH->first = FH->totalFlames[i].next;
						}
						(FH->totalFlames[i].next)->prev = FH->totalFlames[i].prev;
						if(FH->totalFlames[i].prev != NULL)
						{
							(FH->totalFlames[i].prev)->next = FH->totalFlames[i].next;
						}
						FH->last->next = &FH->totalFlames[i];
						FH->totalFlames[i].prev = FH->last;
					}
					else
					{
						FH->first = FH->totalFlames[i].prev;
						FH->totalFlames[i].next = FH->last;
					}

					FH->last = &FH->totalFlames[i];
					FH->last->next = NULL;
					strcpy(page->data,FH->last->data);
					return RC_OK;;
				}
				if(FH->totalFlames[i].pageNum == NO_PAGE)
				{
					FH->first = FH->totalFlames;
					FH->first->prev =NULL;
					if( i==0)
					{
						FH->last = FH->first;
					}
					FH->totalFlames[i].pageNum = pageNum;
					pf->fixCount++;
					page->pageNum = pageNum;
					readBlock(pageNum,&fh,FH->last->data); 
					FH->numReadIO++;
					strcpy(page->data,FH->last->data);
					if(FH->last->next != NULL)
					{
						FH->last = FH->last->next;
					}
					return RC_OK;
				}
			}	
			PageFlame * tmp;
			tmp = FH->first;
			FH->first = FH->first->next;
			(FH->last)->next = tmp;
			tmp->prev = FH->last;	
			FH->last = tmp;
			FH->last->pageNum = pageNum;
			FH->last->next = NULL;
			FH->first->prev = NULL;
			readBlock(pageNum,&fh,FH->last->data); 
			FH->numReadIO++;
			page->pageNum = FH->last->pageNum;
			(FH->last)->fixCount = 1;
			(FH->last)->isDirty = false;
			return RC_OK;
		case RS_CLOCK:
			return RC_OK;
		case RS_LFU:
			return RC_OK;
		case RS_LRU_K:
			return RC_OK;
	}

}
/************************************************
  get an array of the frame content
  each number of the array is a pageNum
************************************************/
PageNumber *getFrameContents(BM_BufferPool *const bm)
{
	FlameHandle *FH;
	FH = bm->mgmtData;
	int np = bm->numPages;
	PageNumber *pn;
	pn = (PageNumber *)malloc(sizeof(PageNumber)*np);
	for(int i=0; i<np;i++)
	{
		pn[i] = (FH->totalFlames[i]).pageNum;
	}
	return pn;
}

/************************************************
  return an boolean array of the dirty page flags
************************************************/
bool *getDirtyFlags(BM_BufferPool *const bm)
{
	FlameHandle *FH;
	FH = bm->mgmtData;
	int np = bm->numPages;
	bool *dirtyFlags;
	dirtyFlags = (bool *)malloc(sizeof(bool)*np);
	for(int i=0; i<np;i++)
	{
		dirtyFlags[i] = (FH->totalFlames[i]).isDirty;
	}
	return dirtyFlags;
}

/************************************************
  return the fix count of all the page
************************************************/
int *getFixCounts(BM_BufferPool *const bm)
{
	FlameHandle *FH;
	FH = bm->mgmtData;
	int np = bm->numPages;
	int *fixTotal;
	fixTotal = (int *)malloc(sizeof(int)*np);
	for(int i=0; i<np;i++)
	{
		fixTotal[i] = (FH->totalFlames[i]).fixCount;
	}
	return fixTotal;

}

/************************************************
  return a number of the time of file read
************************************************/
int getNumReadIO(BM_BufferPool *const bm)
{
	FlameHandle *FH;
	FH = bm->mgmtData;
	int numRead;
	numRead = FH->numReadIO;
	return numRead;
}
/************************************************
  return a number of the time of file write
************************************************/
int getNumWriteIO(BM_BufferPool *const bm)
{
	FlameHandle *FH;
	FH = bm->mgmtData;
	int numWrite;
	numWrite = FH->numWriteIO;
	return numWrite;
}
