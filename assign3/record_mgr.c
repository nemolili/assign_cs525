#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include "storage_mgr.h"
#include "buffer_mgr.h"
#include "record_mgr.h"


/*******************************************/
/*struct for the handle for scan           */
/*                                         */
/***************************************** */
typedef struct Scan_Handle
{
	int page;
	int slot;
	Expr *expr;
	char *data;
} Scan_Handle;
/*******************************************/
/*struct for the header of each page       */
/*                                         */
/***************************************** */
typedef struct Page_Head
{
	int num_PageRecord;//count the number of record in every page
	int free_page[10];//reserved ten free page
} Page_Head;

RC initRecordManager(void *mgmtData)
{
	return RC_OK;	
}
RC shutdownRecordManager()
{
	return RC_OK;
}
/*******************************************/
/*function for the table create            */
/*                                         */
/***************************************** */
RC createTable (char *name, Schema *schema)
{
	createPageFile(name);
	FILE *fp = NULL;
	fp = fopen(name,"r+");
	fseek(fp,0,SEEK_SET);     //write schema to the first page of table
	fwrite(&(schema->numAttr),sizeof(int),1,fp);
	fwrite(schema->attrNames,sizeof(char*),schema->numAttr,fp);
	fwrite(schema->dataTypes,sizeof(DataType),schema->numAttr,fp);
	fwrite(schema->typeLength,sizeof(int),schema->numAttr,fp);
	fwrite(&(schema->keySize),sizeof(int),schema->keySize,fp);
	fwrite(schema->keyAttrs,sizeof(int),schema->keySize,fp);

	fclose(fp);
	fp = NULL;

	return RC_OK;
}
/*******************************************/
/*function to open the table               */
/*connect to the Buffer Managment          */
/***************************************** */
RC openTable( RM_TableData *rel, char *name)
{
	BM_BufferPool *bp;
	bp = (BM_BufferPool *)malloc(sizeof(BM_BufferPool));
	int i;
	int m;
	int keySize;
	char **temcpNames = (char**)malloc(sizeof(char*)*m);
	DataType *temcpDt = (DataType *)malloc(sizeof(DataType)*m);
	int *temcpSizes = (int *)malloc(sizeof(int)*m);
	int *temcpKeys = (int *)malloc(sizeof(int));

	rel->name = name;
	FILE *fp = NULL;
	fp = fopen(name,"r+");
	fseek(fp,0,SEEK_SET);  //read schema from the table
	fread(&(m),sizeof(int),1,fp);
	fread(temcpNames,sizeof(char*),m,fp);
	fread(temcpDt,sizeof(DataType),m,fp);
	fread(temcpSizes,sizeof(int),m,fp);
	fread(&keySize,sizeof(int),1,fp);
	fread(temcpKeys,sizeof(int),keySize,fp);
	rel->schema = createSchema(m,temcpNames,temcpDt,temcpSizes,keySize,temcpKeys);
	// if bufferPool are never been created than create the bufferPool
	if(th.numRecord && (BM_BufferPool*)(rel->mgmtData))
	{
		bp = (BM_BufferPool *)rel->mgmtData;
	}
	else 
	{
		initBufferPool(bp,name,60,RS_FIFO,NULL);
		rel->mgmtData = bp;
	}
	return RC_OK;
}
RC closeTable(RM_TableData *rel)
{
	return RC_OK;
}
/*******************************************/
/*function to delete the table             */
/*destroy the page file                    */
/*******************************************/
RC deleteTable(char *name)
{
	if(destroyPageFile(name) != RC_OK)
	{
		return RC_DESTROY_TABLE_FAILED;
	}
	th.numRecord = 0;
	th.first_FreePage = th.num_FirstFreePage = 0;
	return RC_OK;
}
/*******************************************/
/*return the number of tuples;             */
/*******************************************/
int getNumTuples(RM_TableData *rel)
{
	return th.numRecord;
}

/**********************************************************/
/*calculate the length of the record based on the schema  */
/**********************************************************/
int recordLength(RM_TableData *rel)
{
	int recordLen=0;
	DataType *local_dt;
	local_dt = rel->schema->dataTypes;
	int i;
	// cover every attrbute of the schema
	for(i=0;i<rel->schema->numAttr;i++)
	{
		if(local_dt[i] == DT_INT)
		{
			recordLen += sizeof(int);
		}
		else if(local_dt[i] == DT_STRING)
		{
			recordLen += (rel->schema->typeLength[i])*sizeof(char);
		}
		else if(local_dt[i] == DT_FLOAT)
		{
			recordLen += sizeof(float);
		}
		else if(local_dt[i] == DT_BOOL)
		{
			recordLen += sizeof(bool);
		}
	}
	return recordLen;
}
/**********************************************************/
/*   function to insert record to the table               */
/**********************************************************/
RC insertRecord(RM_TableData *rel, Record *record)
{
	SM_FileHandle fh;
	BM_PageHandle *ph;
	BM_BufferPool *bp;
	int recordLen = recordLength(rel);
	Page_Head pHead;
	ph = (BM_PageHandle *)malloc(sizeof(BM_PageHandle));
	bp = (BM_BufferPool *)rel->mgmtData;
	int i;
	openPageFile(rel->name,&fh);

	//decide wheter need a new page to store the record
	if(!th.numRecord || th.first_FreePage == PAGE_SIZE)
	{
		appendEmptyBlock(&fh);	
		if(th.num_FirstFreePage)
		{
			pinPage(bp,ph,th.num_FirstFreePage);
		}
		else
		{
			//the first page to store the record
			pinPage(bp,ph,1);
			th.numRecord = 0;
			th.first_FreePage = PAGE_SIZE;
			th.num_FirstFreePage = 1;

		}
		// store the ID information to the record
		record->id.page = th.num_FirstFreePage;
		record->id.slot = PAGE_SIZE-recordLen;
		pHead.num_PageRecord = 0;
		// store the data of the record to the table
		memcpy(ph->data+th.first_FreePage-recordLen,record->data,recordLen);
		//get the header information of the page and revise 
		pHead.num_PageRecord = 1;
		th.numRecord =1;
		pHead.free_page[0] = PAGE_SIZE-recordLen;
		for(i=0;i<9;i++)
		{
			pHead.free_page[i+1] = 0;
		}
		memcpy(ph->data,&pHead.num_PageRecord,sizeof(int));
		for(i=0;i<10;i++)
		{
			memcpy(ph->data+sizeof(int)+sizeof(int)*i,&(pHead.free_page[i]),sizeof(int));
		}

		//srite the TID information after the header of page
		memcpy(ph->data+sizeof(pHead),&(record->id.page),sizeof(int));
		memcpy(ph->data+sizeof(pHead)+sizeof(int),&(record->id.slot),sizeof(int));
		th.first_FreePage = th.first_FreePage - recordLen;
		//mark the page dirty and unpin the page
		markDirty(bp,ph);
		unpinPage(bp,ph);
		//free(bp);
		//bp = NULL;
		free(record->data);
		record->data = NULL;
		free(ph->data);
		ph->data = NULL;
		free(ph);
		ph = NULL;
		closePageFile(&fh);
	}
	else 
	{
		//the current page has the free page to store a new record
		pinPage(bp,ph,th.num_FirstFreePage);
		char *tp;
		tp = (char*)malloc(sizeof(int));
		memcpy(tp,ph->data,sizeof(int));
		pHead.num_PageRecord = *(int *)(tp);
		for( i=0;i<10;i++)
		{
			memcpy(tp,ph->data+sizeof(int)+sizeof(int)*i,sizeof(int));
			pHead.free_page[i] = *(int *)(tp);
		}
		//decide whick free spacement to store the record
		if(pHead.free_page[1])
		{
			record->id.page = th.num_FirstFreePage;
			record->id.slot = pHead.free_page[0] - recordLen;
			memcpy(ph->data+th.first_FreePage-recordLen,record,recordLen);
			for(i=0; i<9;i++)
			{
				pHead.free_page[i] = pHead.free_page[i+1];
			}
			pHead.free_page[9] = 0;
			//update the header information
			memcpy(ph->data+sizeof(pHead)+pHead.num_PageRecord*sizeof(RID),&(record->id.page),sizeof(int));
			memcpy(ph->data+sizeof(pHead)+pHead.num_PageRecord*sizeof(RID)+sizeof(int),&(record->id.slot),sizeof(int));
			pHead.num_PageRecord++;
			memcpy(ph->data,&pHead.num_PageRecord,sizeof(int));
			for(i=0;i<10;i++)
			{
				memcpy(ph->data+sizeof(int)+i*sizeof(int),&pHead.free_page[i],sizeof(int));
			}
			th.numRecord++;
			th.first_FreePage = pHead.free_page[0];
			markDirty(bp,ph);
			unpinPage(bp,ph);
			//free(bp);
			//free(ph);
			//bp = NULL;
			//ph = NULL;
			free(record->data);
			record->data = NULL;
			free(ph->data);
			ph->data = NULL;
			free(ph);
			ph = NULL;
			closePageFile(&fh);
		}
		else
		{
			memcpy(ph->data+th.first_FreePage-recordLen,record->data,recordLen);	
			pHead.free_page[0] = pHead.free_page[0]-recordLen;
			record->id.page = th.num_FirstFreePage;
			record->id.slot = pHead.free_page[0];
			pHead.num_PageRecord++;
			memcpy(ph->data,&pHead.num_PageRecord,sizeof(int));
			for( i=0;i<10;i++)
			{
				memcpy(ph->data+sizeof(int)*(i+1),&pHead.free_page[i],sizeof(int));
			}
			memcpy(ph->data+sizeof(pHead)+(pHead.num_PageRecord-1)*sizeof(RID),&(record->id.page),sizeof(int));
			memcpy(ph->data+sizeof(pHead)+(pHead.num_PageRecord-1)*sizeof(RID)+sizeof(int),&(record->id.slot),sizeof(int));

			th.numRecord++;
			th.first_FreePage = pHead.free_page[0];
			markDirty(bp,ph);
			unpinPage(bp,ph);
			//decide the current page whether still have enough free spacement to store a new record
			if((th.first_FreePage-sizeof(pHead)-sizeof(RID)*pHead.num_PageRecord)<(sizeof(RID)+recordLen))
			{
				th.num_FirstFreePage++;
				th.first_FreePage = PAGE_SIZE;
			}
			free(record->data);
			record->data = NULL;
			free(ph->data);
			ph->data = NULL;
			free(ph);
			ph = NULL;
			closePageFile(&fh);
		}
	}
	return RC_OK;
}
/**********************************************************/
/*       the function to delete the record                */
/**********************************************************/
RC deleteRecord(RM_TableData *rel, RID id)
{
	SM_FileHandle fh;
	BM_PageHandle *ph;
	BM_BufferPool *bp;
	int recordLen = recordLength(rel);
	Page_Head pHead;
	ph = (BM_PageHandle *)malloc(sizeof(BM_PageHandle));
	bp = (BM_BufferPool *)rel->mgmtData;
	openPageFile(rel->name,&fh);
	if(id.page > fh.totalNumPages)
	{
		return RC_RM_RECORD_NOT_DELETE;
	}
	pinPage(bp,ph,id.page);

	//get the header thing of the page
	char *tp;
	tp = (char*)malloc(sizeof(int));
	memcpy(tp,ph->data,sizeof(int));
	pHead.num_PageRecord = *(int *)(tp);
	int i;
	for(i=0;i<10;i++)
	{
		memcpy(tp,ph->data+sizeof(int)+sizeof(int)*i,sizeof(int));
		pHead.free_page[i] = *(int *)(tp);
	}
	free(tp);
	tp = NULL;
	//update the value of heder information
	for(i=9;i>0;i--)
	{
		pHead.free_page[i] = pHead.free_page[i-1];
	}
	pHead.free_page[0]= id.slot+recordLen;

	//replace empty string to the deleted record
	char *temp;
	temp = (char *)malloc(sizeof(char)*recordLen);
	memcpy(ph->data+id.slot,temp,recordLen);
	pHead.num_PageRecord--;
	memcpy(ph->data,&pHead.num_PageRecord,sizeof(int));
	for( i=0;i<10;i++)
	{
		memcpy(ph->data+sizeof(int)+sizeof(int)*i,&(pHead.free_page[i]),sizeof(int));
	}
	th.numRecord--;
	//update the free spacement
	if(th.num_FirstFreePage == id.page)
	{
		th.first_FreePage = pHead.free_page[0];
	}
	markDirty(bp,ph);
	unpinPage(bp,ph);
	free(temp);
	temp = NULL;
	free(ph);
	ph = NULL;
	return RC_OK;
}
/**********************************************************/
/*       the function to update the record                */
/**********************************************************/
RC updateRecord(RM_TableData *rel, Record *record)
{
	//SM_FileHandle fh;
	BM_PageHandle *ph;
	BM_BufferPool *bp;
	int recordLen = recordLength(rel);
	ph = (BM_PageHandle *)malloc(sizeof(BM_PageHandle));
	bp = (BM_BufferPool *)rel->mgmtData;
	//openPageFile(rel->name,&fh);
	pinPage(bp,ph,record->id.page);
	memcpy(ph->data+record->id.slot,record->data,recordLen);
	markDirty(bp,ph);
	unpinPage(bp,ph);
	free(ph);
	ph = NULL;
	return RC_OK;
}
/**********************************************************/
/*       the function to get the record                   */
/**********************************************************/
RC getRecord(RM_TableData *rel, RID id, Record *record)
{
	SM_FileHandle fh;
	BM_PageHandle *ph;
	BM_BufferPool *bp;
	int recordLen = recordLength(rel);
	record->data = (char *)malloc(sizeof(char)*recordLen);
	ph = (BM_PageHandle *)malloc(sizeof(BM_PageHandle));
	bp = (BM_BufferPool *)rel->mgmtData;
	openPageFile(rel->name,&fh);
	pinPage(bp,ph,id.page);
	memcpy(record->data,(ph->data+id.slot),recordLen);
	record->id.page = id.page;
	record->id.slot = id.slot;

	unpinPage(bp,ph);
	free(ph);
	ph = NULL;
	closePageFile(&fh);
	return RC_OK;
}

/**********************************************************/
/*       the function to start  the scan function         */
/**********************************************************/
RC startScan(RM_TableData *rel, RM_ScanHandle *scan, Expr *cond)
{
	BM_PageHandle *ph;
	BM_BufferPool *bp;
	int recordLen = recordLength(rel);
	ph = (BM_PageHandle *)malloc(sizeof(BM_PageHandle));
	bp = (BM_BufferPool *)rel->mgmtData;
	
	if(!scan)
	{
		return RC_RM_NO_SCAN;
	}
	pinPage(bp,ph,1);
	Scan_Handle *scanHandle;
	scanHandle = (Scan_Handle *)malloc(sizeof(Scan_Handle));	
	scanHandle->page = 1;
	scanHandle->slot = PAGE_SIZE-recordLen;
	scanHandle->expr = cond;
	scanHandle->data = (char *)malloc(sizeof(char)*PAGE_SIZE);
	memcpy(scanHandle->data,ph->data,PAGE_SIZE);

	scan->rel  = rel;
	scan->mgmtData = scanHandle;
	free(ph);
	ph = NULL;
	return RC_OK;
}
/**********************************************************/
/*       the function to get next tuple satisfy the expr  */ 
/**********************************************************/
RC next(RM_ScanHandle *scan, Record *record)
{
	SM_FileHandle fh;
	BM_PageHandle *ph;
	BM_BufferPool *bp;
	Value *result;
	Record *temp;
	temp = (Record *)malloc(sizeof(Record));
	ph = (BM_PageHandle *)malloc(sizeof(BM_PageHandle));
	bp = (BM_BufferPool *)scan->rel->mgmtData;
	openPageFile(scan->rel->name,&fh);
	int totalNumPages = fh.totalNumPages;
	closePageFile(&fh);
	Page_Head pHead;
	Scan_Handle *scanHandle;
	int recordLen = recordLength(scan->rel);
	scanHandle = (Scan_Handle *)scan->mgmtData;
	char *tp;
	tp = (char*)malloc(sizeof(int));
	memcpy(tp,scanHandle->data,sizeof(int));
	pHead.num_PageRecord = *(int *)(tp);
	free(tp);
	tp = NULL;
	int endSlot;
	endSlot = *(int *)(scanHandle->data+sizeof(pHead)+pHead.num_PageRecord*sizeof(RID)-sizeof(int));
	//if(scanHandle->page == fh.totalNumPages-1)
	if(scanHandle->page == totalNumPages-1)
	{
		if(scanHandle->slot<endSlot)
		{
			free(ph);
			ph = NULL;
			return RC_RM_NO_MORE_TUPLES;
		}
	}	
	if(scanHandle->expr == NULL)
	{
		if(scanHandle->slot<endSlot)
		{
			scanHandle->page++;
			scanHandle->slot = PAGE_SIZE-recordLen;
			pinPage(bp,ph,scanHandle->page);
			memcpy(scanHandle->data,ph->data,PAGE_SIZE);
			unpinPage(bp,ph);
		}
		record->id.page = scanHandle->page;
		record->id.slot = scanHandle->slot;
		memcpy(record->data,scanHandle->data+scanHandle->slot,recordLen);
	}
	else
	{
		RID id;
		do
		{
			if(scanHandle->slot<endSlot)
			{
				scanHandle->page++;
				scanHandle->slot = PAGE_SIZE-recordLen;
				//if(scanHandle->page>=fh.totalNumPages)
				if(scanHandle->page>=totalNumPages)
				{
					free(ph);
					ph = NULL;
					return RC_RM_NO_MORE_TUPLES;
				}
			}
			id.page = scanHandle->page;
			id.slot = scanHandle->slot;
			getRecord(scan->rel,id,temp);
			evalExpr(temp,scan->rel->schema,scanHandle->expr,&result);
			if(result->v.boolV)
			{
				*(record) =*(temp);
				scanHandle->slot = scanHandle->slot-recordLen;
				break;
			}
			scanHandle->slot = scanHandle->slot-recordLen;
		}while(1);
	}
	free(ph);
	ph = NULL;
	return RC_OK;
}
/**********************************************************/
/*       the function to close  the scan function         */
/**********************************************************/
RC closeScan(RM_ScanHandle *scan)
{
	free(scan->mgmtData);
	scan->mgmtData = NULL;
	return RC_OK;
}
/**********************************************************/
/*       the function to get the size of the record       */  
/**********************************************************/
int getRecordSize(Schema *schema)
{
	int recordLen=0;
	DataType *local_dt;
	local_dt = schema->dataTypes;
	int i;
	for(i=0;i<schema->numAttr;i++)
	{
		if(local_dt[i] == DT_INT)
		{
			recordLen += sizeof(int);
		}
		else if(local_dt[i] == DT_STRING)
		{
			recordLen += (schema->typeLength[i])*sizeof(char);
		}
		else if(local_dt[i] == DT_FLOAT)
		{
			recordLen += sizeof(float);
		}
		else if(local_dt[i] == DT_BOOL)
		{
			recordLen += sizeof(bool);
		}
	}
	return recordLen;
}

/**********************************************************/
/*       the function to create  the schema function      */
/**********************************************************/
Schema *createSchema( int numAttr, char **attrNames, DataType *dataTypes, int *typeLength, int keySize, int *keys)
{
	Schema *result;
	result = (Schema *)malloc(sizeof(Schema));
	result->numAttr = numAttr;
	result->attrNames = attrNames;
	result->dataTypes = dataTypes;
	result->typeLength = typeLength;	
	result->keySize = keySize;	
	result->keyAttrs = keys;	
	return result;
}
/**********************************************************/
/*       the function to free schema                     */
/**********************************************************/
RC freeSchema(Schema *schema)
{
	free(schema->attrNames);
	free(schema->dataTypes);
	free(schema->typeLength);
	free(schema->keyAttrs);
	free(schema);
	schema->attrNames = NULL;
	schema->dataTypes = NULL;
	schema->typeLength = NULL;
	schema->keyAttrs = NULL;
	schema = NULL;
	return RC_OK;
}
/**********************************************************/
/*       the function to create a record                  */
/**********************************************************/
RC createRecord(Record **record, Schema *schema)
{
	Record *pieceRecord;
	RID id;
	char *data;
	
	pieceRecord = (Record *)malloc(sizeof(Record));
	id = *(RID *)malloc(sizeof(id));
	data = (char *)malloc(sizeof(char));
	pieceRecord->id = id;
	pieceRecord->data = data;
	*record = pieceRecord;
	return RC_OK;
}
/**********************************************************/
/*       the function to free the record function         */
/**********************************************************/
RC freeRecord(Record *record)
{
	free(record->data);
	free(record);
	return RC_OK;
}
/**********************************************************/
/*       the function to get attribute of the record      */
/**********************************************************/
RC getAttr(Record *record, Schema *schema, int attrNum, Value **value)
{
	int pos = 0;
	DataType *dt;	
	dt = schema->dataTypes;	
	Value *val;
	val = (Value *)malloc(sizeof(Value));
	int i;
	for(i=0;i<attrNum;i++)
	{
		if(dt[i] == DT_INT)
		{
			pos += sizeof(int);
		}
		else if(dt[i] == DT_STRING)
		{
			pos += (schema->typeLength[i])*sizeof(char);
		}
		else if(dt[i] == DT_FLOAT)
		{
			pos += sizeof(float);
		}
		else if(dt[i] == DT_BOOL)
		{
			pos += sizeof(bool);
		}
	}
	val->dt = dt[attrNum];
	switch(val->dt)
	{
		case DT_INT:
			val->v.intV = *(int *)(record->data+pos);
			break;
		case DT_STRING:
			val->v.stringV = malloc((schema->typeLength[attrNum])*sizeof(char));
			memcpy(val->v.stringV,record->data+pos,(schema->typeLength[attrNum])*sizeof(char));
			break;
		case DT_FLOAT:
			val->v.floatV = *(float *)(record->data+pos);
			break;
		case DT_BOOL:
			val->v.boolV = *(bool *)(record->data+pos);
			break;
	}
	*(value) = val;
	return RC_OK;
}
/**********************************************************/
/*       the function to set attribute of the record      */
/**********************************************************/
RC setAttr(Record *record, Schema *schema, int attrNum, Value *value)
{
	int pos = 0;
	DataType *dt;
	dt = schema->dataTypes;
	int i;
	for(i=0;i<attrNum;i++)
	{
		if(dt[i] == DT_INT)
		{
			pos += sizeof(int);
		}
		else if(dt[i] == DT_STRING)
		{
			pos += (schema->typeLength[i])*sizeof(char);
		}
		else if(dt[i] == DT_FLOAT)
		{
			pos += sizeof(float);
		}
		else if(dt[i] == DT_BOOL)
		{
			pos += sizeof(bool);
		}
	}
	switch(value->dt)
	{
		case DT_INT:
			*((int*)(record->data+pos)) = (int)(value->v.intV);
			break;
		case DT_STRING:
			memcpy(record->data+pos,value->v.stringV,4);
			break;
		case DT_FLOAT:
			*((float*)(record->data+pos)) = (float)(value->v.intV);
			break;
		case DT_BOOL:
			*((bool*)(record->data+pos)) = (bool)(value->v.intV);
			break;
	}
	return RC_OK;
}

