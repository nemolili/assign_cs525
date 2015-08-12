The projcet: Record Manager

The author:
	Name:           ID:
	Chuanfan chen   A20320526
	Yuxiang liu     A20319150
	Guanxiong wang  A20321384
	Qinyao XU       A20304397

  This assignment is to creat a record manager. The record manager handles tables with a fixed schema. Clients can insert records, delete records, update records, and scan through the records in a table. A scan is associated with a search condition and only returns records that match the search condition. Each table should be stored in a separate page file and your record manager should access the pages of the file through the buffer manager implemented in the last assignment.
  Record Representation: The data types we consider for this assignment are all fixed length. Thus, for a given schema, the size of a record is fixed too.
  Page Layout: You will have to define how to layout records on pages. Also you need to reserve some space on each page for managing the entries on the page. Refresh your memory on the page layouts discussed in class! For example, how would you represent slots for records on pages and manage free space.
  Table information pages: You probably will have to reserve one or more pages of a page file to store, e.g., the schema of the table.
  Record IDs: The assignment requires you to use record IDs that are a combination of page and slot number.
  Free Space Management: Since your record manager has to support deleting records you need to track available free space on pages. An easy solution is to link pages with free space by reserving space for a pointer to the next free space on each page. One of the table information pages can then have a pointer to the first page with free space. One alternative is to use several pages to store a directory recording how much free space you have for each page.

Table and Record Manager Functions
  Similar to previous assignments, there are functions to initialize and shutdown a record manager. Furthermore, there are functions to create, open, and close a table. Creating a table should create the underlying page file and store information about the schema, free-space, ... and so on in the Table Information pages. All operations on a table such as scanning or inserting records require the table to be opened first. Afterwards, clients can use the RM_TableData struct to interact with the table. Closing a table should cause all outstanding changes to the table to be written to the page file. The getNumTuples function returns the number of tuples in the table.
    

Record Functions
  These functions are used to retrieve a record with a certain RID, to delete a record with a certain RID, to insert a new record, and to update an existing record with new values. When a new record is inserted the record manager should assign an RID to this record and update the record parameter passed to insertRecord.
   
Scan Functions
  A client can initiate a scan to retrieve all tuples from a table that fulfill a certain condition (represented as an Expr). Starting a scan initializes the RM_ScanHandle data structure passed as an argument to startScan. Afterwards, calls to the next method should return the next tuple that fulfills the scan condition. If NULL is passed as a scan condition, then all tuples of the table should be returned. next should return RC_RM_NO_MORE_TUPLES once the scan is completed and RC_OK otherwise (unless an error occurs of course). Below is an example of how a client can use a scan.
    

Schema Functions
  These helper functions are used to return the size in bytes of records for a given schema and create a new schema.
    extern Schema *createSchema (int numAttr, char **attrNames, DataType *dataTypes, int *typeLength, int keySize, int *keys);
    extern RC freeSchema (Schema *schema);

Attribute Functions
  These functions are used to get or set the attribute values of a record and create a new record for a given schema. Creating a new record should allocate enough memory to the data field to hold the binary representations for all attributes of this record as determined by the schema.
 
ERROR_MESSAGE:
#define RC_DESTROY_FILE_FAILED 
#define RC_DESTROY_TABLE_FAILED 
#define RC_RM_RECORD_NOT_DELETE 
#define RC_RM_NO_SCAN 
Note:
  I can complie the whole file on the local server. However it may appear the memory problem on the fourier server;
  on the fourier server, The code can finish the all scan test and testrecords

Extra:
TIDs :Each record will have a TID after the header of the page file when user insert.
      We can locate every record easily.

The structure is :
		Makefile
		buffer_mgr.h
		buffer_mgr_stat.c
		buffer_mgr_stat.h
		dberror.c
		dberror.h
		expr.c
		expr.h
		record_mgr.h
		rm_serializer.c
		storage_mgr.h
		tables.h
		test_assign3_1.c
		test_expr.c
		test_helper.h

Note:
	If you have some question or need to submit bugs, please email to gwang43@hawk.iit.edu.
	Github link: https://github.com/randywhisper/assign_cs525.
